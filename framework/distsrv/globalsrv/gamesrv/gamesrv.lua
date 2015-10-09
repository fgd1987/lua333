module('Gamesrv', package.seeall)
portfd = nil

game_manager = game_manager or {}
game_session = game_session or {}

function main()
    portfd = Port.create(Framesrv.loop)
    listen()
end

function ev_read(sockfd, reason)
    log('ev_read sockfd(%d)', sockfd)
    local err = Srvproto.dispatch(sockfd)
    if err then
        Port.close(portfd, sockfd)
    end
end

function ev_close(sockfd, reason)
    log('ev_close sockfd is %d', sockfd)
end

function select(srvname)
    local gamesrv = game_manager[srvname]
    if not gamesrv then
        logerr('srvname(%s) not found', srvname)
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
--@srvname 服务名称
function REGIST(sockfd, srvname)
    if game_manager[srvname] ~= nil then
        logerr('%s is connected yet', srvname)
        return 
    end
    local srv = {
        srvname = srvname,
        sockfd = sockfd,
        time = os.time()
    }
    game_manager[srvname] = srv 
    game_session[sockfd] = srv 
    log('a game regist srvname(%s)', srvname)
end
