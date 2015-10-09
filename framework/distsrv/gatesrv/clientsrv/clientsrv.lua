module('Clientsrv', package.seeall)

portfd = nil

socket_manager = socket_manager or {}
tmp_socket_manager = tmp_socket_manager or {}

function main()
    portfd = Port.create(Framesrv.loop)
    Pbc.import_dir(Config.clientsrv.protodir)
    listen()
end

function ev_read(sockfd, reason)
    log('ev_read sockfd(%d)', sockfd)
    local err, msgname, msg = Pbproto.decode(sockfd)
    if err then
        Port.close(portfd, sockfd)
        return
    end
    local pats = string.split(msgname, '.')
    local mod_name = pats[1]
    local action_name = pats[2]
    local timenow = os.time()
    local player = socket_manager[sockfd]
    local route = Config.clientsrv.route[mod_name]
    route = route or Config.clientsrv.route[msgname]
    --防止刷包
    if player then
        player.packet_counter = player.packet_counter + 1
        if player.packet_counter > Config.clientsrv.packet_count_threshold then
            if timenow - player.last_check_packet_time < Config.clientsrv.packet_time_threshold then
                logwarn('packet hack')
                local msg = pbc.msgname('login.DISCONNECT')
                msg.errno = 14
                Pbproto.send(player.sockfd,  msg)
                disconnect(player.sockfd, 'packet hack')
                return
            end
            player.last_check_packet_time = timenow
            player.packet_counter = 0
        end
    end
    --验证
    if not player and  mod_name == 'login' and action_name == 'LOGIN' then
        local uid = msg.uid
        if uid == nil then 
            logerr('uid not found, sockfd(%d)', sockfd)
            return
        end
        Port.setuid(portfd, sockfd, uid)
        Login.player_connected(sockfd, msg)
        return
    end
    --转发到global_srv的优化级更高
    if route then
        --末认证
        if route.need_auth and not player then
            logerr('not auth')
            return
        end
        --自己处理
        if route.target == 'self' then
            _G[string.cap(mod_name)]['MSG_'..action_name](player, msg)
            return
        end
        local uid = player and player.uid or -sockfd
        local sockfd = nil
        if type(route.target) == 'string' then
            sockfd = _G[route.target]
        elseif type(route.target) == 'table' then
            local srv_name = route.target[math.random(1, #route.target)]
            sockfd = GlobalSrv.select(srv_name)
        end
        FORWARD(sockfd, uid, msg)
        return
    end
    --验证了的玩家才能转发消息到game_srv
    if player and not route then
        Gameclient.forward(player.srv_name, player.uid, msg)
        return
    end
end

function disconnect(sockfd, reason) 
    Port.close(portfd, sockfd, reason);
end

function ev_close(sockfd, host, port, reason)
    log('ev_close sockfd(%d)', sockfd)
    local player = socket_manager[sockfd]
    if player then
        log('client close from uid(%d) ip(%s) reason(%s)', player.uid, host, reason)
        --保存数据 
        Login.player_disconnect(player)
        player.sockfd = nil
        socket_manager[sockfd] = nil
        return
    end
    if tmp_socket_manager[sockfd] then
        tmp_socket_manager[sockfd] = nil
    end
end

function ev_accept(sockfd, host, port)
    log('accept a client sockfd(%d) host(%s) port(%d)', sockfd, host, port)
    tmp_socket_manager[sockfd] = {time = os.time()}
end

function listen()
    log('listen on host(%s) port(%d)', Config.clientsrv.host, Config.clientsrv.port)
    Port.rename(portfd, "Clientsrv")
    if not Port.listen(portfd, Config.clientsrv.port) then
        error('listen fail')
    end
    Port.on_accept(portfd, 'Clientsrv.ev_accept')
    Port.on_close(portfd, 'Clientsrv.ev_close')
    Port.on_read(portfd, 'Clientsrv.ev_read')
end

--功能:定时检测链接上来后, 没有发消息登陆的socket
function timer_check()
    local timenow = os.time()
    for sockfd, v in pairs(tmp_socket_manager) do
        local timebefore = v.time
        if timenow - timebefore > Config.clientsrv.tmp_sock_idle_sec then
            --太久没有验证成功的socket, 要关闭了
            Port.close(portfd, sockfd, 'tmp to long')
            log('tmp to long ip(%s)', Port.getpeerip(sockfd))
        end
    end
    return 1
end

function reply(sockfd, msg)
    Pbproto.send(sockfd, msg)
end
