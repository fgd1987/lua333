module('Pbtest', package.seeall)
--[[
    参数有点多，各个游戏用脚本跑吧 
    pbtest game/jxqy/pbtest uid
    pbtest game/jxqy/pbtest/login/login uid
--]]

function run()

end

function run_mod()

end

function run_func()

end

function send(player, msgname, params)
    local msg = pbc.msgnew(msgname)
    for k, v in pairs(params) do
        msg[k] = v
    end
    local sockfd = player.sockfd
    if not sockfd then
        sockfd = Socket.socket(Socket.AF_INET, Socket.SOCK_STREAM, 0)
        local conf = Config.clientsrv
        Socket.connect(sockfd, conf.host, conf.port)
        if sockfd < 0 then
            print('connect fail')
            os.exit(1)
        end
        player.sockfd = sockfd
        Sendbuf.create(sockfd)
        Recvbuf.create(sockfd, 10240)
    end
    local sent = Pbproto.send(sockfd, msg)
    Log.log(TAG, 'sockfd(%d) sent(%d)', sockfd, sent)
    while sent > 0 do
        Sys.sleepmsec(1)
        sent = sent - Pbproto.flush(sockfd)
    end
    Log.log(TAG, 'send msg(%s) success', pbc.msgname(msg))
end

--接收消息
function recv(player, msgname)
    while true do
        local _, msgname, msg = Pbproto.decode(player.sockfd)
        if  msg then
            print(string.format('recv msg name(%s)', pbc.msgname(msg))) 
            if pbc.msgname(msg) == msgname then
                return msg
            end
        end
    end
end
