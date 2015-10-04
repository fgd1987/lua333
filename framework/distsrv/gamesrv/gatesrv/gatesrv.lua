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
--@srv_name 服务名称
function SRV_ONLINE(sockfd, srv_name)
    if gate_manager[srv_name] ~= nil then
        logerr('game(%s) is connected yet', srv_name)
        return 
    end
    local srv = {
        srv_name = srv_name,
        sockfd = sockfd,
        time = os.time()
    }
    gate_manager[srv_name] = srv 
end
