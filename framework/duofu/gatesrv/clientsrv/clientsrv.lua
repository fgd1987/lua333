module('ClientSrv', package.seeall)

listenfd = listen or nil
pollfd = pollfd or nil
socket_table = socket_table or {}

function main()
    pollfd = Poll.create()
    listenfd = Socket.create()
    local conf = Config.gateway.client
    Socket.listen(listenfd, conf.ip, conf.port)
    Socket.setnonblock(listenfd)
end

function update()
    local sockfd = Socket.accept(listenfd)
    if sockfd then
        socket_table[sockfd] = {LAST_FRAME_TIME + Config.gatesrv.client.check_interval}
        Poll.add(pollfd, sockfd)
    end
    while true do
        local sockfd, event = Poll.poll(pollfd)
        local sockfd, revent, wevent, eevent = Poll.poll(pollfd)
        local sockfd, read_event, write_event, error_event = Poll.poll(pollfd)
        if not sockfd then break end
        local socket_info = socket_table[sockfd]
        local error = nil
        if read_event then
            error = ProtobufDecoder.dispatch(sockfd)
        end
        if write_event then
            error = ProtobufEncoder.flush(sockfd)
        end
        if error_event then
            error = true
        end
        if error then
            socket_table[sockfd] = nil
            Log.log(TAG, 'disconnect from ip(%s) reason(%s)', Socket.getpeerip(sockfd), os.last_errorstr())
        end
        if not error then
            socket_info.next_check_time = LAST_FRAME_TIME + Config.gatesrv.client.check_interval 
        end
    end
    check_connections()
end

function check_connections()
    for socket, info in pairs(socket_table) do
        if LAST_FRAME_TIME > info.next_check_time then
            Socket.close(sockfd)
        end
    end
end

function destory()
    Poll.destory(pollfd)
end

function close_socket(sockfd)
    if not socket_table[sockfd] then
        Log.log(TAG, 'sockfd not found sockfd(%d)', sockfd)
        return
    end
    Socket.close(sockfd)
end
