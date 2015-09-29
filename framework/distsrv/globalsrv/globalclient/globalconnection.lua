module('GlobalClient', package.seeall)

connection_table = connection_table or {}

function main()
    local globalsrv_list = Config.globalsrv_list
    for _, conf in pairs(globalsrv_list) do
        local sockfd = Socket.create()
        Socket.rename(sockfd, conf.alias)
        connection_table.insert(sockfd)
    end
end

function update()
    for _, sockfd in pairs(connection_table) do
        if not Socket.isconnect(sockfd) then
            Socket.connect(sockfd)
            if Socket.isconnect(sockfd) then
            end
        end
        if Socket.iserror(sockfd) then
            Log.log(TAG, 'disconnect from %s reason(%s)', Socket.getpeerip(sockfd), Socket.error_string(sockfd))
        end
    end
    for _, sockfd in pairs(connection_table) do
        if Socket.isconnect(sockfd) then
            RpcDecoder.dispatch(sockfd)
        end
    end
end

