module('GlobalSrv', package.seeall)

connection_table = connection_table or {}

function main()
    local conf = Config.globalsrv.globalsrv
    local sockfd = SocketSrv.create()
    SocketSrv.listen(sockfd, conf.ip, conf.port)
end

function update()
    local sockfd = SocketSrv.accept()
    if sockfd then
        local sockfd = Socket.create(sockfd)
        Socket.rename(sockfd, MOD_NAME)
        connection_table.insert(sockfd)
        Log.log(TAG, 'accept a client from %s', Socket.getpeerip(sockfd))
    end
    for _, sockfd in pairs(connection_table) do
        RpcDecoder.dispatch(sockfd)
    end
end

function check_connection()
    for _, sockfd in pairs(connection_table) do
        if Socket.iserror(sockfd) then
            Log.log(TAG, 'disconnect from %s reason(%s)', Socket.getpeerip(sockfd), Socket.error_string(sockfd))
            connection_table[sockfd] = nil
        end
    end
end




