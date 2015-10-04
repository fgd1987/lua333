module('Gameclient', package.seeall)

portfd = portfd or nil
socket_table = socket_table or {}

function main()
    portfd = Port.create(Framesrv.loop)
    Port.rename(portfd, 'Gameclient')
    Port.on_close(portfd, 'Gameclient.ev_close')
    Port.on_connect_err(portfd, 'Gameclient.ev_connect_err')
    Port.on_connect_suc(portfd, 'Gameclient.ev_connect_suc')
    Port.on_read(portfd, 'Gameclient.ev_read')
    check_connections()
end

function ev_read(sockfd)
    log('ev_read sockfd(%d)', sockfd)
    local err = Srvproto.dispatch(sockfd)
    if err then
        Port.close(portfd, sockfd)
    end
end

function ev_connect_err(sockfd, host, port)
    log('ev_connect_err sockfd(%d)', sockfd)
    socket_table[sockfd] = nil
    check_connections()
end

function update()
end

function ev_connect_suc(sockfd, host, port)
    log('ev_connect_suc sockfd(%d)', sockfd)
    POST(sockfd, 'Login.login', host, port, {'hello', 'world'})
end

function ev_close(sockfd)
    log('ev_close sockfd(%d)', sockfd)
    socket_table[sockfd] = nil
    check_connections ();
end

--重连
function check_connections()
    local gamesrv_list = Config.gameclient.gamesrv_list
    for sockfd, info in pairs(socket_table) do
        local find = false
        for index, conf in pairs(gamesrv_list) do
            if conf.host == info.host and conf.port == info.port then
                find = true
                break
            end
        end
        if not find then
            log('config changed!!')
            Socket.close(socket_table[index].sockfd, 'config changed')
        end
    end
    for index, conf in pairs(gamesrv_list) do
        local find = false
        for sockfd, info in pairs(socket_table) do
            if conf.host == info.host and conf.port == info.port then
                find = true
                break
            end
        end
        if not find then
            local sockfd = Port.connect(portfd, conf.host, conf.port)
            log('to connect sockfd(%d) host(%s) port(%d)', sockfd, conf.host, conf.port)
            if sockfd then
                socket_table[sockfd] = {sockfd = sockfd, host = conf.host, port = conf.port}
            end
        end
    end
end
