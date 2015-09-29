module('Login', package.seeall)

TAG = 'Login'
SAVE_INTERVAL = 10
logout_player_table = logout_player_table or {}

--上线
function _LOGIN(sockfd, uid)
    POST(CENTER_SRV).Login._LOGIN(uid)
end

--下线
function _LOGOUT(sockfd, uid)
    local player = Player.get_player(uid)
    if not player then
        POST(sockfd).Login._LOGOUT(uid)
        return
    end
    Player.logout(player, 'logout')
end

function _LOGIN_PASS(sockfd, uid)
    POST(DB_SRV).Login._LOAD(uid)
end

--退出
function _LOGOUT_PASS(sockfd, uid)
    local player = Player.get_player(uid)  
    if not player then
        Log.error(TAG, '')
        return
    end
end

--顶下线
function _INSTEAD(sockfd, uid)
    local player = Player.get_player(uid)
    if not player then
        POST(sockfd).Login._LOGOUT(uid)
        return
    end
    Player.logout(player, 'instead')
end

--数据返回
function _LOAD_RETURN(sockfd, uid)
    local player = Player.get_player(uid)
    if player then
        Log.error(TAG, 'repeat login')
        return
    end
    local player = Player
end

--保存返回
function _SAVE_RETURN(sockfd, uid, error, reason)
    if error ~= 0 then
        Log.error(TAG, 'save return error(%d) reason(%d)', error, reason)
        return
    end
    POST(player.sockfd).Login._LOGOUT(uid)
    logout_player_table[uid] = nil
end

--下线
function logout(player)
    Player.del_player(uid)
    logout_player_table[uid] = player
    player.next_save_time = System.time() + SAVE_INTERVAL
    POST(DB_SRV).Login._SAVE(uid, data)
end

--定时器
function update()
    for _, player in pairs(logout_player_table) do
        if not player.next_save_time or System.time() > player.next_save_time then
            Log.error(TAG, 'save timeout uid(%d)', player.uid)
            logout(player)
        end
    end
end
