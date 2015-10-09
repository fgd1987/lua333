module('Globalsrv', package.seeall)

portfd = nil

global_manager = {}

function main()
    portfd = Port.create(Framesrv.loop)
    listen()
end

function ev_close(sockfd, reason)
    log('ev_close sockfd(%d)', sockfd)
end

function ev_read(sockfd)
    log('ev_read sockfd(%d)', sockfd)
    local err = Strproto.dispatch(sockfd)
    if err then
        Port.close(portfd, sockfd)
    end
end

function ev_accept(sockfd)
    log('accept a global sockfd(%d)', sockfd)
end

function listen()
    log('listen on host(%s) port(%d)', _CONF.host, _CONF.port)
    Port.rename(portfd, "GlobalSrv")
    if not Port.listen(portfd, _CONF.port) then
        error('listen fail')
    end
    Port.on_accept(portfd, 'Globalsrv.ev_accept')
    Port.on_close(portfd, 'Globalsrv.ev_close')
    Port.on_read(portfd, 'Globalsrv.ev_read')
end

--功能:globalsrv上线
--@srv_name 服务名称
function SRV_ONLINE(sockfd, srv_name)
    if global_manager[srv_name] ~= nil then
        logerr('%s is connected yet', srv_name)
        return 
    end
    local srv = {
        srv_name = srv_name,
        sockfd = sockfd,
        time = os.time()
    }
    global_manager[srv_name] = srv 
end
