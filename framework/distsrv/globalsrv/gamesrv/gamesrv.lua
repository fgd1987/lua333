module('Gamesrv', package.seeall)
portfd = nil

game_manager = game_manager or {
    --[srvid] = {}
}
game_session = game_session or {
    --[sockfd] = {srvname = '', sockfd = 0, time = 0}
}

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
    for k, game in pairs(game_manager) do
        if game.sockfd == sockfd then
            game_manager[k] = nil
            log('game disconnect srvname(%s)', game.srvname)
            break
        end
    end
end

function select(srvid)
    local game = game_manager[srvnid]
    if not game then
        logerr('srvid(%s) not found', srvid)
        return
    end
    return game.sockfd
end

function ev_accept(sockfd)
    log('accept a game sockfd(%d)', sockfd)
end

function listen()
    log('listen on host(%s) port(%d)', _CONF.host, _CONF.port)
    Port.rename(portfd, "GameSrv")
    if not Port.listen(portfd, _CONF.port) then
        error('listen fail')
    end
    Port.on_accept(portfd, 'Gamesrv.ev_accept')
    Port.on_close(portfd, 'Gamesrv.ev_close')
    Port.on_read(portfd, 'Gamesrv.ev_read')
end


--功能:game_srv上线
--@srvname 服务名称
function REGIST(sockfd, srvid, srvname)
    if game_manager[srvid] ~= nil then
        logerr('%s is connected yet', srvname)
        return 
    end
    local srv = {
        srvid = srvid,
        srvname = srvname,
        sockfd = sockfd,
        time = os.time()
    }
    game_manager[srvid] = srv 
    game_session[sockfd] = srv 
    log('a game regist srvname(%s)', srvname)
end
