module('Gatesrv', package.seeall)

listenfd = listen or nil
pollfd = pollfd or nil

connection_table = connection_table or {}

function main()
    local pollfd = Poll.create()
    local conf = Config.gamesrv.gateway
    listenfd = Socket.create()
    Socket.listen(listenfd, conf.ip, conf.port)
    Socket.setnonblock(listenfd)
end

function update()
    local sockfd = Socket.accept(listenfd)
    if sockfd then
        Poll.add(pollfd, sockfd)
    end
    while true do
        local sockfd, event = Poll.select(pollfd)
        if not sockfd then break end
        local error = nil
        if event == Poll.EV_ERROR then
            error = Poll.EV_ERROR
        elseif event == Poll.EV_READ then
            error = ProtobufDecoder.dispatch(sockfd)
        elseif event == Poll.EV_WRITE then
            error = ProtobufEncoder.flush(sockfd)
        end
        if error then
            Poll.del(pollfd, sockfd)
            Log.log(TAG, 'disconnect from ip(%s) reason(%s)', Socket.getpeerip(sockfd), os.last_errorstr())
        end
    end
end

function destory()
    Poll.destory(pollfd)
end
