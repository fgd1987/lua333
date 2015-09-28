module('Gameclient', package.seeall)

pollfd = nil
socket_array = socket_array or {}
socket_table = socket_table or {}
connecting_socket_array = connecting_socket_array or {}
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

--重连
function check_connections()
    local gamesrv_list = Config.gameclient.gamesrv_list
    for k, conf in pairs(gamesrv_list) do
        --查看是否已经连接，或者正在连接
        if not socket_table[k] and not connecting_socket_table[k] then
            --去连接
            local sockfd = Socket.socket(Socket.AF_INET, Socket.SOCK_STREAM, 0)
            Socket.setnonblock(sockfd)
            local error = Socket.connect(sockfd, conf.host, conf.port)
            Log.log(TAG, 'connect to sockfd(%d) host(%s) port(%d) error(%d)', sockfd, conf.host, conf.port, error)
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
                connecting_socket_table[k] = {host = conf.host, port = conf.port, sockfd = sockfd, idx = k}
            end
        end
        --如果地址已经变化，马上关闭
        if socket_table[k] and (socket_table[k].host ~= conf.host or socket_table[k].port ~= conf.port) then
            Log.log('config changed!!')
            Socket.close(socket_table[k].sockfd)
        end
    end
end

function on_write(sockfd)
    Log.log(TAG, 'on_write')
end

function on_read(sockfd)
    Log.log(TAG, 'on_read')
end

function on_connect_suc(sockfd)
    --[[
     if(true || getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len) != 0){
        LOG_ERROR("connect success, but sockfd %d, error %d, strerror:%s", sockfd, error,  strerror(errno));
    }else if(error == 128){
        LOG_LOG("operation now in progress");
        return;
    }
    else if(error != 0){
        LOG_ERROR("connect success, but sockfd %d, error %d, strerror:%s", sockfd, error,  strerror(errno));
        line->status = LINE_STATUS_DISCONNECT;
        port_close_line(linefd, strerror(errno));
        return;
    }
    --]]
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



