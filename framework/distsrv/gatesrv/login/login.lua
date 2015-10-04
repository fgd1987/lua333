module('Login', package.seeall)

player_manager = player_manager or {}

function main()

end

--功能：进入游戏服
function MSG_ENTER(player, msg)
    local srv_name = msg.srv_name
    if not srv_name or srv_name == '' then
        srv_name = Gameclient.randselect() 
    end
    local sockfd = Gameclient.select(srv_name)
    if not sockfd then
        logerr('game_srv(%s) not found')
        msg.errno = 11
        Gameclient.reply(player.sockfd, msg)
        return
    end
    player.srv_name = srv_name
    POST(sockfd, 'Login.PLAYER_ENTER', player.uid)
end

--功能：进入游戏服,切换服务器
function PLAYER_ENTER(sockfd, uid, srv_name)
    local player = player_manager[uid]
    if not player then
        logerr('player is offline uid(%d)', uid)
        return
    end
    local sockfd = Gameclient.select(srv_name)
    if not sockfd then
        loggerr('game_srv(%s) not found')
        return
    end
    player.srv_name = srv_name
    POST(sockfd, 'Login.PLAYER_ENTER', player.uid)
end

--功能:登陆
function player_connected(sockfd, msg)
    local uid = msg.uid
    if uid == nil or uid <= 0 then
        msg.errno = 8
        Clientsrv.reply(sockfd, msg)
        Clientsrv.close(sockfd, 'uid is wrong')
        return
    end
    --验证
    if not check_auth(uid, msg.params) then
        msg.errno = 10
        Clientsrv.reply(sockfd, msg)
        Clientsrv.close(sockfd, 'auth fail')
        return
    end
    --加锁
    local player = {sockfd = sockfd, uid = uid}
    player_manager[uid] = player
    Clientsrv.socket_manager[sockfd] = player
    Clientsrv.tmp_socket_manager[sockfd] = nil
    msg.errno = 0
    Clientsrv.reply(sockfd, msg)
    return true 
end

--功能：下线
function player_disconnect(player)
    local sockfd = _G[player.sockname]
    if not sockfd then
        logerr('user offline yet uid(%d)', player.uid)
        return
    end
    POST(sockfd, 'login.PLAYER_EXIT', player.uid)
end


function check_login_token(uid, params)
    if not Config.auth then
        return true
    end
    local kv = {}
    local params = string.split(params, '&')
    for _, param in pairs(params) do
        local pats = string.split(param, '=')
        kv[pats[1]] = os.urldecode(pats[2])
    end
    local time = tonumber(kv.time)
    if Config.sign_expire_sec ~= 0 and os.time() - time / 1000 >= Config.sign_expire_sec then
        logger:error(string.format('user(%d) sign time expire, recv(%d) now(%d)', uid, time, os.time()))
        return false
    end
    local kset = {}
    for k, v in pairs(kv) do
        table.insert(kset, k)
    end
    table.sort(kset)
    local str = ''
    for _, k in pairs(kset) do
        if k ~= 'sig' then
            str = str..k..'='..kv[k]
        end
    end
    str = str..Config.srv_secret
    local token = os.md5(str)
    if token ~= kv.sig or uid ~= tonumber(kv.uid) then
        logger:error(string.format('user(%d) auth fail sign(%s) time(%s) uid(%d) token(%s)', uid, kv.sig, kv.time, kv.uid, token))
        return false
    end
    return true
end
