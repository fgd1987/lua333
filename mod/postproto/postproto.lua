module('Postproto', package.seeall)

function dispatch(sockfd, proto, funcname, ...)
    local pats = string.split(funcname, '.')
    local mod = _G[pats[1]]
    if not mod then
        logerr('mod(%s) not found', pats[1])
        return
    end
    local func = mod[pats[2]]
    if not func then
        logerr('func(%s) not found', pats[1])
        return
    end
    func(sockfd, ...)
end

_G['POST'] = function(sockfd, funcname, ...)
    Srvproto.send(sockfd, Srvproto.POST_PROTO, funcname, ...)
end
