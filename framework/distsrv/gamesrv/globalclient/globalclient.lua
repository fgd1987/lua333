module('Globalclient', package.seeall)

portfd = nil
socket_table = socket_table or {}

function main()
    portfd = Port.create(Framesrv.loop)
    Port.rename(portfd, "Globalclient")
    Port.on_close(portfd, 'Globalclient.ev_close')
    Port.on_connect_err(portfd, 'Globalclient.ev_connect_err')
    Port.on_connect_suc(portfd, 'Globalclient.ev_connect_suc')
    Port.on_read(portfd, 'Globalclient.ev_read')
    check_connections()
end

function ev_read(sockfd)
    log('ev_read sockfd(%d)', sockfd)
end

function ev_connect_err(sockfd, host, port)
    socket_table[sockfd] = nil
    check_connections()
end

function ev_connect_suc(sockfd, host, port)
    log('ev_connect_suc sockfd(%d)', sockfd)
    local globalsrv_list = Config.globalclient.globalsrv_list
    for index, conf in pairs(globalsrv_list) do
        if conf.host == host and port == conf.port then
            _G[conf.alias] = sockfd  
            log('alias sockfd(%d) to alias(%s)', sockfd, conf.alias)
            break
        end
    end
    POST(sockfd, 'Gamesrv.REGIST', Config.srvconf.srvname)
    --POST(sockfd, 'Gamesrv.REGIST', Config.srvconf.srvname)
    POST(sockfd, 'Dbsrv.GET', 333, 'hello', 'user')
end

function ev_close(sockfd, reason)
    log('ev_close sockfd(%d) reason(%s)', sockfd, reason)
    local globalsrv_list = Config.globalclient.globalsrv_list
    for index, conf in pairs(globalsrv_list) do
        if conf.host == host and port == conf.port then
            _G[conf.alias] = nil
            log('del sockfd(%d) to alias(%s)', sockfd, conf.alias)
            break
        end
    end
    socket_table[sockfd] = nil
    check_connections()
end

--重连
function check_connections()
    local globalsrv_list = Config.globalclient.globalsrv_list
    for sockfd, info in pairs(socket_table) do
        local find = false
        for index, conf in pairs(globalsrv_list) do
            if conf.host == info.host and conf.port == info.port then
                find = true
                break
            end
        end
        if not find then
            log('config changed!!')
            Port.close(socket_table[index].sockfd, 'config changed')
        end
    end
    for index, conf in pairs(globalsrv_list) do
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
