module('Pbtest', package.seeall)
--[[
    参数有点多，各个游戏用脚本跑吧 
    pbtest game/jxqy/pbtest uid
    pbtest game/jxqy/pbtest/login/login uid
--]]

local portfd = nil
local loop = nil

function connect(player)
    if not loop then
        loop = Ae.create(10240)
    end
    if not portfd then
        portfd = Port.create(loop)
        Port.rename(portfd, 'pbtest')
        Port.on_close(portfd, 'Pbtest.ev_close')
        Port.on_read(portfd, 'Pbtest.ev_read')
    end
end

function ev_read(sockfd, reason)
    log('ev_read')
end

function ev_close(sockfd, host, port, reason)
    log('ev_close')
end


function send(player, msgname, params)
    local msg = pbc.msgnew(msgname)
    for k, v in pairs(params) do
        msg[k] = v
    end
    local sockfd = player.sockfd
    if not sockfd then
        local conf = Config.pbtest
        sockfd = Port.syncconnect(portfd, conf.host, conf.port)
        if sockfd < 0 then
            log('connect fail')
            os.exit(1)
        end
        player.sockfd = sockfd
    end
    Pbproto.send(sockfd, msg)
    log('send msg(%s) success', pbc.msgname(msg))
end

--接收消息
function recv(player, msgname)
    while true do
        Ae.run_once(loop)
        local _, msgname, msg = Pbproto.decode(player.sockfd)
        if  msg then
            log(string.format('recv msg name(%s)', pbc.msgname(msg))) 
            if pbc.msgname(msg) == msgname then
                log(pbc.debug_string(msg))
                return msg
            end
        end
    end
end
