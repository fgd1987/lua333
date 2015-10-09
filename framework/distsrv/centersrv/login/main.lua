module('Login', package.seeall)

player_manager = player_manager or {}
onlinenum = onlinenum or 0

function main()

end

--功能:某个玩家上线
--@uid 玩家uid
function PLAYER_ENTER(sockfd, uid)
    local game = Gamesrv.game_session[sockfd]
    if not game then
        logerr('sockfd(%d) is not found', sockfd)
        return
    end
    local player = player_manager[uid]
    local srvid = game.srvid
    if not player then
        log('player online uid(%d)', uid)
        --插入一条记录
        player = {
                uid = uid,
                srvname = game.srvname, 
                srvid = game.srvid,
                srvname_before = nil,
                time = os.time(),
                sockfd = sockfd,
                }
        player_manager[uid] = player
        --通知可以上线
        POST(game.sockfd, 'Login.PLAYER_PASS', uid)
        game.onlinenum = game.onlinenum + 1
        onlinenum = onlinenum + 1
        game.is_sync = true
    elseif player.srvid then
        log('player instead to from game(%s) to game(%s)', game.srvname, player.srvname)
        --通知这个服下线
        POST(Gamesrv.select(player.srvid), 'Login.PLAYER_INSTEAD', uid, game.srvname)
        --记录下这个player的srv_name
        player.instead_srvid = game.srvid
    else
        logerr('srvname is not found srvname(%s)', game.srvname)
    end
end

--功能:某个玩家下线
--@uid 玩家uid
function PLAYER_EXIT(sockfd, uid)
    local game = Gamesrv.game_session[sockfd]
    if not game then
        logerr('sockfd is not found sockfd(%d)', sockfd)
        return
    end
    log('player(%d) offline', uid)
    local player = player_manager[uid]
    local srvid = game.srvid
    if not player then
        log('player is not online uid(%d)', uid)
        return
    end
    if player.srvid ~= game.srvid then
        logerr('player.srvid(%s) diff from srvid(%s)', player.srvid, game.srvid)
        return
    end
    local instead_srvid = player.instead_srvid
    if instead_srvid ~= nil then
        --顶掉对方
        local instead_game = Gamesrv.game_manager[instead_srvid]
        --通知之前的服
        POST(Gamesrv.select(instead_srv_name)).Login.PLAYER_PASS(uid)
        player.sockfd = instead_game.sockfd
        player.srv_name = instead_srv_name
        player.instead_srv_name = nil
        game.onlinenum = game.onlinenum - 1
        game.is_sync = true
        instead_game.onlinenum = instead_game.onlinenum + 1
        instead_game.is_sync = true
    else
        --真的下线 
        player_manager[uid] = nil
        game.onlinenum = game.onlinenum - 1
        onlinenum = onlinenum - 1
        game.is_sync = true
    end
end
