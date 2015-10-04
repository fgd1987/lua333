module('Login', package.seeall)

function main()
--    Evdisp.add_timer(global_timer, Config.game_srv.save_interval * 1000, "Login.timer_check")
end

--功能:进入服务器
function PLAYER_ENTER(sockfd, uid)
    Runtime.tmp_player_manager[uid] = sockfd
    logger:log(uid, 'player enter')
    POST(CenterSrv.sockfd, uid, 'login.PLAYER_ENTER', uid)
end

--功能:退出服务器
function PLAYER_EXIT(sockfd, uid)
    logger:log(uid, 'player exit')
    local player = Runtime.player_manager[uid]
    if not player then
        logger:error(uid, 'player out found')
        POST(CenterSrv.sockfd, uid, 'login.PLAYER_EXIT', uid)
        return
    end
    player_logout(player)
end

--功能：可以上线
function PLAYER_PASS(sockfd, uid)
    local player = Runtime.player_manager[uid]
    if player then
        logger:error(uid, 'player online yet')
        return
    end
    --开始加载数据
    DbSrv.get(uid, 'Login.msg_db_srv_get_playerdata', Config.user_tables)
end

--功能:被顶号
function PLAYER_INSTEAD(sockfd, uid)
    logger:log(uid, 'player exit')
    --解锁
    local tmp_player = Runtime.tmp_player_manager[uid]
    if tmp_player_manager then
        Runtime.tmp_player_manager[uid] = nil
    end
    
    local player = Runtime.player_manager[uid]
    if not player then
        logger:error(uid, 'player out found')
        return
    end
    local msg = pbc.msgnew('login.DISCONNECT') 
    msg.errno = 15
    player:post_msg(msg)
    player_logout(player)
end

--功能:登陆成功
--@player
function player_login(player)
    local msg = pbc.msgnew('login.ENTER')

    local uid = player.uid
    local sockfd = player.sockfd
    local timenow = os.time()
    local user = player.playerdata.user
    local last_login_time = user.last_login_time
    local rt, err = pcall(Player.init, player)
    if not rt then
        logger:log(err)
    end
    if not os.issameday(last_login_time, timenow) then
        local rt, err = pcall(Player.reset, player)
        if not rt then
            logger:error(err)
        end
    end
    if not os.issameweek(last_login_time, timenow) then
        local rt, err = pcall(Player.reset_per_week, player)
        if not rt then
            logger:error(uid, err)
        end
    end
    if not os.issamemonth(last_login_time, timenow) then
        local rt, err = pcall(Player.reset_per_month, player)
        if not rt then
            logger:error(uid, err)
        end
    end
    user.last_login_time = timenow

    --版署防沉迷
    if false and Guard.is_tired(player) then
        msg.errno = 12
        player:post_msg(msg)
        return
    end
    --[[
     --还未注册的新手状态
    if user.regist_time == 0 then
        msg.errno = 13
        player:post_msg(msg)
        return
    end
    --]]
    local ip_addr = Port.getpeerip(sockfd)
    logger:db(10001, player.uid, user.is_vip, Config.srv_id, ip_addr, user.regist_time, Runtime.onlinenum, ip_addr)
    player:post_msg(msg)
end

--功能:下线
--player
function player_logout(player)
    local uid = player.uid
    local rt, err = pcall(Player.close, player)
    if not rt then
        logger:error(uid, err)
    end
    --保存数据
    local playerdata = player.playerdata
    local user_tables = {}
    for table_name, table_msg in pairs(playerdata) do
        --是否有脏标志
        if table_msg:is_dirty() then
            logger:log(uid, string.format('table_name(%s) is dirty', table_name))
            user_tables[table_name] = table_msg
            table_msg:set_dirty(0)
        else
            logger:log(uid, string.format('table_name(%s) is clean', table_name))
        end
    end
    --保存失败
    if not DbSrv.set(uid, 'Login.msg_db_srv_set_playerdata', user_tables) then
        for table_name, table_msg in pairs(user_tables) do
            table_msg:set_dirty(1)
        end
    end
end


--功能:保存数据dv_srv返回
--@msg db_srv.SET_REPLY
function msg_db_srv_set_playerdata(msg)
    local uid = msg.uid
    local player = Runtime.player_manager[uid]
    if not player then
        logger:error(uid, 'player offline yet')
        return 
    end
    local playerdata = player.playerdata
    local user = playerdata.user

    local table_statuses = msg.table_statuses
    local count = table_statuses:count()
    local fail_count = 0
    for i = 1, count do
        local table = table_statuses:get(i)
        local table_name = table.table_name
        if table.status ~= 'OK' then
            logger:error(uid, string.format('table_name(%s) set data fail', table_name))
            playerdata[table_name]:set_dirty(1)
            fail_count = fail_count + 1
        end
    end
    local timenow = os.time()
    --已经下线了
    if fail_count == 0 then
        logger:db(20001, player.uid, user.is_vip, Config.srv_id, (timenow - user.last_login_time), user.regist_time, player.last_scene_id)
        --要这样才能释放数据??
        for k, v in pairs(playerdata) do
            playerdata[k] = nil
        end
        player.playerdata = nil
        --释放数据
        Runtime.player_manager[uid] = nil
        Runtime.onlinenum = Runtime.onlinenum - 1
        --通知gateway_srv下线
        POST(CenterSrv.sockfd, uid, 'login.PLAYER_EXIT', uid)
    end
end


--功能:读取数据返回
--@uid 
--@user_tables {table_name = msg}
--@argback string
function msg_db_srv_get_playerdata(uid, user_tables, argback)
    --如果没有锁住
    local sockfd = Runtime.tmp_player_manager[uid]
    if not sockfd then
        POST(CenterSrv.sockfd, uid, 'login.PLAYER_EXIT', uid)
        logger:error(uid, 'player is disconnect')
        return
    end
    local player = Runtime.player_manager[uid]
    if player then
        player.sockfd = sockfd
        logger:error(uid, 'player is online yet')
        return
    end

    for table_name, table_msg in pairs(user_tables) do
        if table_msg == false then
            POST(CenterSrv.sockfd, uid, 'login.PLAYER_EXIT', uid)
            logger:error(uid, string.format('load %d.%s fail', uid, table_name))
            return
        end
    end

    local playerdata = user_tables
    logger:log(uid, '加载数据成功')
    --建立session
    local player = Player.PlayerClass:new(sockfd, uid)
    Runtime.player_manager[uid] = player
    Runtime.tmp_player_manager[uid] = nil
    Runtime.onlinenum = Runtime.onlinenum + 1
    player.playerdata = playerdata

    local rt, err = pcall(player_login, player)
    if not rt then
        logger:error(uid, err)
        player_logout(player)
    end
end


--功能:定时保存玩家数据
function timer_check()
    local player_manager = Runtime.player_manager
    local timenow = os.time()
    for uid, player in pairs(player_manager) do
        local playerdata = player.playerdata
        if timenow - player.last_save_time > Config.game_srv.save_interval then
            local table_names = {}
            local tables = {}
            for table_name, msg in pairs(playerdata) do
                if msg:is_dirty() then
                    tables[table_name] = msg
                    msg:set_dirty(0)
                else
                    logger:log(uid, string.format('table_name(%s) is clean'))
                end
            end
            --保存失败
            if not DbSrv.set(uid, 'Login.msg_db_srv_set_playerdata', tables) then
                for _, table_name in pairs(table_names) do
                    playerdata[table_name]:set_dirty(1)
                end
            else
                player.last_save_time = timenow
            end
        end
    end
    return Config.game_srv.save_interval * 1000
end

