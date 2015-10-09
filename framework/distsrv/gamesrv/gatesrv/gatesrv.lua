module('Gatesrv', package.seeall)

portfd = port or nil

gate_manager = {}

function main()
    portfd = Port.create(Framesrv.loop)
    listen()
end

function ev_read(sockfd)
    log('ev_read sockfd(%d)', sockfd)
    local err = Srvproto.dispatch(sockfd)
    if err then
        Port.close(portfd, sockfd)
    end
end

function ev_close(sockfd, reason)
    log('ev_close sockfd(%d) reason(%s)', sockfd, reason)
    for _, gate in pairs(gate_manager) do
        if gate.sockfd == sockfd then
            gate_manager[sockfd] = nil
            break
        end
    end
end

function select(srv_name)
    local gate = gate_manager[srv_name]
    if not gate then
        logerr('srv_name(%s) not found', srv_name)
        return
    end
    return gate.sockfd
end

function ev_accept(sockfd)
    log('accept a gate sockfd(%d)', sockfd)
end

function listen()
    log('listen on host(%s) port(%d)', Config.gatesrv.host, Config.gatesrv.port)
    Port.rename(portfd, "Gatesrv")
    if not Port.listen(portfd, Config.gatesrv.port) then
        error('listen fail')
    end
    Port.on_accept(portfd, 'Gatesrv.ev_accept')
    Port.on_close(portfd, 'Gatesrv.ev_close')
    Port.on_read(portfd, 'Gatesrv.ev_read')
end

--功能:game_srv上线
--@srvname 服务名称
function REGIST(sockfd, srvname)
    if gate_manager[srvname] ~= nil then
        logerr('game(%s) is connected yet', srvname)
        return 
    end
    local srv = {
        srvname = srvname,
        sockfd = sockfd,
        time = os.time()
    }
    gate_manager[srvname] = srv 
    log('a gate regist srvname(%s)', srvname)
end
