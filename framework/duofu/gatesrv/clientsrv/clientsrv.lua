module('Clientsrv', package.seeall)


listenfd = listen or nil
pollfd = pollfd or nil
socket_table = socket_table or {}

function test()
    Log.log(TAG, 'i am clientsrv %s', '333')
end

function main()
    --TODO 配置
    pollfd = Ae.create(65536)
    listenfd = Socket.socket(Socket.AF_INET, Socket.SOCK_STREAM, 0)
    if listenfd < 0 then
        Log.error(TAG, 'socket fail')
        os.exit(1)
    end
    local conf = Config.clientsrv
    Log.log(TAG, 'listen on host(%s) port(%d)', conf.host, conf.port)
    if Socket.listen(listenfd, conf.host, conf.port) < 0 then
        Log.error(TAG, 'listen fail host(%s) port(%d)', conf.host, conf.port)
        os.exit(1)
    end
    if Socket.setnonblock(listenfd) < 0 then
        Log.error('setnonblock fail')
        os.exit(1)
    end
    if Ae.create_file_event(pollfd, listenfd, Ae.READABLE, 'Clientsrv.on_read') ~= Ae.OK then
        Log.error(TAG, 'create file event fail sockfd(%d)', listenfd)
        os.exit(1)
    end
    update()
end

function on_write(sockfd)
    Log.log(TAG, 'on_write sockfd(%d)', sockfd)
    --跑帧的话，可写事件就一直加在SOCKET里吧，不会出现CPU空转的情况的
    --但这样可能效果会不高， 因为每帧都会通知所有SOCKET可写事件
    local error = Strproto.flush(sockfd)
    if error then
        socket_table[sockfd] = nil
        Log.log(TAG, 'disconnect from ip(%s) reason(%s)', Socket.getpeerip(sockfd), os.last_errorstr())
    end
end

function on_read(sockfd)
    Log.log(TAG, 'on_read sockfd(%d)', sockfd)
    if sockfd == listenfd then
        local sockfd = Socket.accept(listenfd)
        if Socket.setnonblock(listenfd) < 0 then
            Log.error(Tag, 'set nonblock sockfd(%d)', sockfd)
            Socket.close(sockfd)
            return
        end
        --加事件
        if Ae.create_file_event(pollfd, sockfd, Ae.READABLE, 'Clientsrv.on_read') ~= Ae.OK then
            Log.error(Tag, 'create file event fail sockfd(%d)', sockfd)
            Socket.close(sockfd)
            return
        end
        if Ae.create_file_event(pollfd, sockfd, Ae.WRITABLE, 'Clientsrv.on_write') ~= Ae.OK then
            Log.error(Tag, 'create file event fail sockfd(%d)', sockfd)
            Socket.close(sockfd)
            return
        end
        --全话
        socket_table[sockfd] = {next_check_time = Framesrv.time + Config.clientsrv.check_interval}
        --buf
        Recvbuf.create(sockfd, 1024)
        Sendbuf.create(sockfd)
        return
    end
    local socket_info = socket_table[sockfd]
    socket_info.next_check_time = Framesrv.time + Config.clientsrv.check_interval 
    local error = Strproto.dispatch(sockfd)
    if error then
        socket_table[sockfd] = nil
        Log.log(TAG, 'disconnect from ip(%s) reason(%s)', Socket.getpeerip(sockfd), os.last_errorstr())
    end
end

function update()
    Log.log(TAG, 'update')
    Ae.run_once(pollfd)
    check_connections()
end

function check_connections()
    for socket, info in pairs(socket_table) do
        if Framesrv.time > info.next_check_time then
            Socket.close(sockfd)
        end
    end
end

function destory()
    Poll.free(pollfd)
end

function close_socket(sockfd)
    if not socket_table[sockfd] then
        Log.log(TAG, 'sockfd not found sockfd(%d)', sockfd)
        return
    end
    Socket.close(sockfd)
end
