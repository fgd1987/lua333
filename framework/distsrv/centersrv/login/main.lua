module('Login', package.seeall)

player_dict = player_dict or {}
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
    local player = player_dict[uid]
    local srv_name = game.srv_name
    if not player then
        log('player online uid(%d)', uid)
        --插入一条记录
        player = {
                uid = uid,
                srv_name = srv_name, 
                srv_name_before = nil,
                time = os.time(),
                sockfd = sockfd,
                }
        player_dict[uid] = player
        --通知可以上线
        POST(game_srv.sockfd, 'Login.PLAYER_PASS', uid)
        game.onlinenum = game.onlinenum + 1
        onlinenum = onlinenum + 1
        game_srv.is_sync = true
    elseif player.srv_name then
        log('player instead to from game_srv(%s) to game_srv(%s)', srv_name, player.srv_name)
        --通知这个服下线
        POST(Gamesrv.select(player.srv_name), 'Login.PLAYER_INSTEAD', uid, srv_name)
        --记录下这个player的srv_name
        player.instead_srv_name = srv_name
    else
        logerr('srv_name is not found srv_name(%s)', srv_name)
    end
end

--功能:某个玩家下线
--@uid 玩家uid
function PLAYER_EXIT(sockfd, uid)
    local game = Gamesrv.game_dict[sockfd]
    if not game then
        logerr('sockfd is not found sockfd(%d)', sockfd)
        return
    end
    log('player(%d) offline', uid)
    local player = player_dict[uid]
    local srv_name = game.srv_name
    if not player then
        log('player is not online uid(%d)', uid)
        return
    end
    if player.srv_name ~= srv_name then
        logerr('player.srv_name(%s) diff from srv_name(%s)', player.srv_name, srv_name)
        return
    end
    local instead_srv_name = player.instead_srv_name
    if instead_srv_name ~= nil then
        --顶掉对方
        local instead_game_srv = Gamesrv.gamev_manager[instead_srv_name]
        --通知之前的服
        POST(Gamesrv.select(instead_srv_name)).Login.PLAYER_PASS(uid)
        player.sockfd = instead_game_srv.sockfd
        player.srv_name = instead_srv_name
        player.instead_srv_name = nil
        game_srv.onlinenum = game_srv.onlinenum - 1
        game_srv.is_sync = true
        instead_game_srv.onlinenum = instead_game_srv.onlinenum + 1
        instead_game_srv.is_sync = true
    else
        --真的下线 
        player_dict[uid] = nil
        game.onlinenum = game.onlinenum - 1
        onlinenum = onlinenum - 1
        game.is_sync = true
    end
end
