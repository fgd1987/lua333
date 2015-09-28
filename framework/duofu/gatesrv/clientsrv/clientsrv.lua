module('Clientsrv', package.seeall)
--[[
主动关闭socket时，优化一下，不要调用close, 要调用shutdown rd 将读端关闭，然后将全部数据发送后，再close
--]]
listenfd = listen or nil
pollfd = pollfd or nil
socket_session = socket_session or {}

function test()
    Log.log(TAG, 'i am clientsrv %s', '333')
end

function main()
    pollfd = Ae.create(Config.clientsrv.maxsock)
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
    if Ae.create_file_event(pollfd, listenfd, Ae.READABLE, 'Clientsrv.onread') ~= Ae.OK then
        Log.error(TAG, 'create file event fail sockfd(%d)', listenfd)
        os.exit(1)
    end
    --加载协议
    Pbc.import_dir(Config.clientsrv.protodir)
end

function onwrite(sockfd)
    Log.log(TAG, 'onwrite sockfd(%d)', sockfd)
    --跑帧的话，可写事件就一直加在SOCKET里吧，不会出现CPU空转的情况的
    --但这样可能效果会不高， 因为每帧都会通知所有SOCKET可写事件
    local sent = Pbproto.flush(sockfd)
    if sent <= 0 then
        Ae.delete_file_event(pollfd, sockfd, Ae.WRITABLE)
    end
end

function onclose(sockfd)
    socket_session[sockfd] = nil
    Log.log(TAG, 'disconnect from ip(%s) reason(%s)', Socket.getpeerip(sockfd), Sys.strerror())
    Sendbuf.free(sockfd)
    Recvbuf.free(sockfd)
    Ae.delete_file_event(pollfd, sockfd, Ae.READABLE)
    Ae.delete_file_event(pollfd, sockfd, Ae.WRITABLE)
    Socket.close(sockfd)
end

function onaccept(listenfd)
    local sockfd = Socket.accept(listenfd)
    if Socket.setnonblock(listenfd) < 0 then
        Log.error(Tag, 'set nonblock sockfd(%d)', sockfd)
        Socket.close(sockfd)
        return
    end
    --加事件
    if Ae.create_file_event(pollfd, sockfd, Ae.READABLE, 'Clientsrv.onread') ~= Ae.OK then
        Log.error(Tag, 'create file event fail sockfd(%d)', sockfd)
        Socket.close(sockfd)
        return
    end
    if Ae.create_file_event(pollfd, sockfd, Ae.WRITABLE, 'Clientsrv.onwrite') ~= Ae.OK then
        Log.error(Tag, 'create file event fail sockfd(%d)', sockfd)
        Socket.close(sockfd)
        return
    end
    --会话
    socket_session[sockfd] = {next_check_time = Framesrv.time + Config.clientsrv.check_interval}
    --buf
    Recvbuf.create(sockfd, 1024)
    Sendbuf.create(sockfd)
end

function onread(sockfd)
    Log.log(TAG, 'onread sockfd(%d)', sockfd)
    if sockfd == listenfd then
        onaccept(listenfd)
        return
    end
    local socket_info = socket_session[sockfd]
    socket_info.next_check_time = Framesrv.time + Config.clientsrv.check_interval 
    local error, msgbuf, msglen, msgname = Pbproto.decodebuf(sockfd)
    if error then
        onclose(sockfd)
        return
    end
    local msg = pbc.msgnew(msgname)
    pbc.parse_from_buf(msg, msgbuf, msglen)
    Log.log(TAG, 'recv msg(%s)', msgname)
    print(pbc.debug_string(msg))
    Recvbuf.buf2line(sockfd)
    --部分消息不用解包，直接发给gamesrv
    reply(sockfd, msg)
end

function reply(sockfd, msg)
    local sent =  Pbproto.send(sockfd, msg)
    if sent <= 0 then
        return
    end
    if Ae.create_file_event(pollfd, sockfd, Ae.WRITABLE, 'Clientsrv.onwrite') ~= Ae.OK then
        Log.error(Tag, 'create file event fail sockfd(%d)', sockfd)
        Socket.close(sockfd)
        return
    end
end

function update()
    --Log.log(TAG, 'update')
    Ae.run_once(pollfd)
    check_connections()
end

function check_connections()
    for socket, info in pairs(socket_session) do
        if Framesrv.time > info.next_check_time then
            Socket.close(sockfd)
        end
    end
end

function destory()
    Poll.free(pollfd)
end

function closesocket(sockfd)
    if not socket_session[sockfd] then
        Log.log(TAG, 'sockfd not found sockfd(%d)', sockfd)
        return
    end
    Socket.close(sockfd)
end
