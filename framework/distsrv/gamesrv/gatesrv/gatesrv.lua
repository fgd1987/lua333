module('Gatesrv', package.seeall)

listenfd = listen or nil
pollfd = pollfd or nil

connection_table = connection_table or {}

function main()
    pollfd = Select.create()
    listenfd = Socket.socket(Socket.AF_INET, Socket.SOCK_STREAM, 0)
    if listenfd < 0 then
        Log.error(TAG, 'socket fail')
        os.exit(1)
    end
    Log.log(TAG, 'listen on sockfd(%d) host(%s) port(%d)', listenfd, Config.gatesrv.host, Config.gatesrv.port)
    if Socket.listen(listenfd, Config.gatesrv.host, Config.gatesrv.port) < 0 then
        Log.error(TAG, 'listen fail error(%s)', Sys.strerror())
        os.exit(1)
    end
    Socket.setnonblock(listenfd)
    Select.add_read_event(pollfd, listenfd)
end

function on_close(sockfd)
    Log.log(TAG, 'on close sockfd(%d)', sockfd)
    Select.remove(pollfd, sockfd)
    Log.log(TAG, 'disconnect from ip(%s) reason(%s)', Socket.getpeerip(sockfd), os.last_errorstr())
    Socket.close(sockfd)
    Sendbuf.free(sockfd)
    Recvbuf.free(sockfd)
    Select.remove(sockfd)
end

function on_accept(listenfd)
    Log.log(TAG, 'on accept sockfd(%d)', listenfd)
    local sockfd = Socket.accept(listenfd)
    if sockfd < 0 then
        return
    end
    Socket.setnonblock(sockfd)
    Sendbuf.create(sockfd, 10240)
    Recvbuf.create(sockfd)
    Select.add_read_event(pollfd, sockfd)
    Select.add_write_event(pollfd, sockfd)
    Log.log(TAG, 'accept a sockfd(%d)', sockfd)
end

function on_read(sockfd)
    Log.log(TAG, 'on read sockfd(%d)', sockfd)
    --local error = ProtobufDecoder.dispatch(sockfd)
end

function on_write(sockfd)
    Log.log(TAG, 'on write sockfd(%d)', sockfd)
    --local error = ProtobufEncoder.flush(sockfd)
end

function update()
    --Log.log(TAG, 'update')
    local numevents = Select.poll(pollfd)
    --Log.log(TAG, 'numevents(%d)', numevents)
    for i = 1, numevents do
        local sockfd, read_ev, write_ev = Select.getevent(pollfd, i)
        if not sockfd then break end
        if sockfd == listenfd then
            on_accept(sockfd)
        elseif read_ev then
            on_read(sockfd)
        elseif write_ev then
            on_write(sockfd)
        end
    end
end

function destory()
    Poll.free(pollfd)
end


