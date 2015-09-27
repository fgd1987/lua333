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
    pollfd = Ae.create()
    check_connections()
end

function update()
    while true do
        local sockfd, read_event, write_event, error_event = Poll.select(pollfd)
        if not sockfd then break end
        if connecting_socket_table[sockfd] then
            local socket_info = connecting_socket_table[sockfd]
            connecting_socket_table[sockfd] = nil
            if read_event then
                --连接成功
                socket_table[socket_info.idx] = socket_info
                Log.log(TAG, 'connect success to ip(%s) port(%d)', ip, port)
            else
                --连接失败
                Log.log(TAG, 'unable connect to ip(%s) port(%d)', ip, port)
            end
        else
            local error = nil
            if error_event then
                error = true
            end
            if read_event then
                error = RpcDecoder.dispatch(sockfd)
            end
            if write_event then
                error = ProtobufEncoder.flush(sockfd)
            end
            if error then
                Log.log(TAG, 'disconnect from %s reason(%s)', Socket.getpeerip(sockfd), Socket.error_string(sockfd))
            end
        end
    end
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
            local error = Socket.connect(sockfd, conf.ip, conf.port)
            if not error then
                Poll.add(pollfd, sockfd)
                connecting_socket_table[k] = {ip = conf.ip, port = conf.port, sockfd = sockfd, idx = k}
            end
        end
        --如果地址已经变化，马上关闭
        if socket_table[k] and (socket_table[k].ip ~= conf.ip or socket_table[k].port ~= conf.port) then
            Socket.close(socket_table[k].sockfd)
        end
    end
end
