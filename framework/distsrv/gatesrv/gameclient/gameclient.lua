module('Gameclient', package.seeall)

pollfd = nil
socket_table = socket_table or {}
connecting_socket_table = connecting_socket_table or {}

function test()
    Log.log(TAG, 'i am gameclientv %s', '333')
end

function main()
    pollfd = Ae.create(1024)
    check_connections()
end

function update()
    Log.log(TAG, 'update')
    Ae.run_once(pollfd)
end

function connect(host, port)
    local sockfd = Socket.socket(Socket.AF_INET, Socket.SOCK_STREAM, 0)
    Socket.setnonblock(sockfd)
    local error = Socket.connect(sockfd, host, port)
    Log.log(TAG, 'connect to sockfd(%d) host(%s) port(%d) error(%d)', sockfd, host, port, error)
    if error == -1 and Sys.errno() == Sys.EINPROGRESS then
        Log.log(TAG, 'create file event sockfd(%d)', sockfd)
        if Ae.create_file_event(pollfd, sockfd, Ae.READABLE, 'Gameclient.on_connect_err') ~= Ae.OK then
            Log.error(Tag, 'create file event fail sockfd(%d)', sockfd)
            Socket.close(sockfd)
            return
        end
        if Ae.create_file_event(pollfd, sockfd, Ae.WRITABLE, 'Gameclient.on_connect_suc') ~= Ae.OK then
            Log.error(Tag, 'create file event fail sockfd(%d)', sockfd)
            Socket.close(sockfd)
            return
        end
        connecting_socket_table[sockfd] = {host = host, port = port, sockfd = sockfd}
    end
end

--重连
function check_connections()
    local gamesrv_list = Config.gameclient.gamesrv_list
    for sockfd, info in pairs(socket_table) do
        local find = false
        for k, conf in pairs(gamesrv_list) do
            if conf.host == info.host and conf.port == info.port then
                find = true
                break
            end
        end
        if not find then
            Log.log(TAG, 'config changed!!')
            Socket.close(socket_table[k].sockfd)
        end
    end
    for k, conf in pairs(gamesrv_list) do
        local find = false
        for sockfd, info in pairs(socket_table) do
            if conf.host == info.host and conf.port == info.port then
                find = true
                break
            end
        end
        for sockfd, info in pairs(connecting_socket_table) do
            if conf.host == info.host and conf.port == info.port then
                find = true
                break
            end
        end
        if not find then
            Log.log(TAG, 'config changed!!')
            connect(conf.host, conf.port)
        end
    end
end

function on_close(sockfd)
    socket_table[sockfd] = nil
    connecting_socket_table[sockfd] = nil
    check_connections ();
end

function on_write(sockfd)
    Log.log(TAG, 'on_write')
end

function on_read(sockfd)
    Log.log(TAG, 'on_read')
end

function on_connect_suc(sockfd)
    Log.log(TAG, 'on_connect_suc sockfd(%d)', sockfd)
    Ae.delete_file_event(pollfd, sockfd, Ae.READABLE)
    Ae.delete_file_event(pollfd, sockfd, Ae.WRITABLE)
    if Ae.create_file_event(pollfd, sockfd, Ae.READABLE, 'Gameclient.on_read') ~= Ae.OK then
        Log.error(Tag, 'create file event fail sockfd(%d)', sockfd)
        Socket.close(sockfd)
        return
    end
    if Ae.create_file_event(pollfd, sockfd, Ae.WRITABLE, 'Gameclient.on_write') ~= Ae.OK then
        Log.error(Tag, 'create file event fail sockfd(%d)', sockfd)
        Socket.close(sockfd)
        return
     end
end

function on_connect_err(sockfd)
    Log.log(TAG, 'on_connect_err')
end