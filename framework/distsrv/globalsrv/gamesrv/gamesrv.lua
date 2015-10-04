module('Gamesrv', package.seeall)
portfd = nil

gamesrv_manager = gamesrv_manager or {}
gamesrv_session = gamesrv_session or {}

function main()
    portfd = Port.create(Framesrv.loop)
    listen()
end

function ev_read(sockfd, reason)
    log('ev_read sockfd is %d', sockfd)
    log('ev_read sockfd(%d)', sockfd)
    local err = Strproto.dispatch(sockfd)
    if err then
        Port.close(portfd, sockfd)
    end
end

function ev_close(sockfd, reason)
    log('ev_close sockfd is %d', sockfd)
end

function select(srv_name)
    local gamesrv = gamesrv_manager[srv_name]
    if not gamesrv then
        logerr('srv_name(%s) not found', srv_name)
        return
    end
    return gamesrv.sockfd
end

function ev_accept(sockfd)
    log('accept a game sockfd(%d)', sockfd)
end

function listen()
    log('listen on host(%s) port(%d)', Config.gamesrv.host, Config.gamesrv.port)
    Port.rename(portfd, "GameSrv")
    if not Port.listen(portfd, Config.gamesrv.port) then
        error('listen fail')
    end
    Port.on_accept(portfd, 'Gamesrv.ev_accept')
    Port.on_close(portfd, 'Gamesrv.ev_close')
    Port.on_read(portfd, 'Gamesrv.ev_read')
end


--功能:game_srv上线
--@srv_name 服务名称
function SRV_ONLINE(sockfd, srv_name)
    if gamesrv_manager[srv_name] ~= nil then
        logerr('%s is connected yet', srv_name)
        return 
    end
    local srv = {
        srv_name = srv_name,
        sockfd = sockfd,
        time = os.time()
    }
    gamesrv_manager[srv_name] = srv 
    gamesrv_session[sockfd] = srv 
end
