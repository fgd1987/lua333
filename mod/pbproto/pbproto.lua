module('Pbproto', package.seeall)

--[[
2byte(packet len) + 2byte(seq) + 2byte(msgname len) + xbyte(msgname) + xbyte(msgbody)
--]]
--拆包
function dispatch(sockfd)
    local msgname, msg = decode(sockfd)
    if not msg then
        Recvbuf.buf2line(sockfd)
        return
    end
    --log('%s %s', msgname, pbc.debug_string(msg))
    local pats = string.split(msgname, '.')
    local modname = pats[1]
    local funcname = pats[2]
    local mod = _G[string.cap(modname)]
    if not mod then
        logerr('mod(%s) not found', modname)
        return
    end
    local func = mod['MSG_'..funcname]
    if not func then
        logerr('func(%s) not found', funcname)
        return
    end
    func()
end

--@return error, msgbuf, msglen, msgname
function decodebuf(sockfd)
    local buf = Recvbuf.getrptr(sockfd)
    local bufremain = Recvbuf.bufremain(sockfd)
    local real_recv = Socket.recv(sockfd, buf, bufremain)
    --log('real_recv(%d)', real_recv)
    if real_recv == 0 or (real_recv == -1 and Sys.errno() == Socket.EAGAIN) then
     --   log('errno(%d) Sys.EAGIN(%d)', Sys.errno(), Socket.EAGAIN)
        return true -- return error
    end
    if real_recv <= 0 then
        return
    end
    Recvbuf.wskip(sockfd, real_recv)
    --log('real_recv(%d) bufremain(%d)', real_recv, bufremain)
    local datalen = Recvbuf.datalen(sockfd) 
    if datalen < 2 then
        --log('header not enough(%d)', datalen)
        return
    end
    local plen = Recvbuf.getint16(sockfd)
    --log('plen(%d)', plen)
    if Recvbuf.datalen(sockfd) < plen then
        --log('data not enough(%d/%d)', datalen, plen)
        return
    end
    local arfd = Ar.create(buf, plen)
    local plen = Ar.readint16(arfd)
    local seq = Ar.readint16(arfd)
    local msgname = Ar.readlstr(arfd)
    local msgbuf = Ar.getptr(arfd)
    local msglen = Ar.remain(arfd)
    --log('plen(%d) seq(%d) msgname(%s) msglen(%d)', plen, seq, msgname, msglen)
    Ar.free(arfd)
    Recvbuf.rskip(sockfd, plen)
    return nil, msgbuf, msglen, msgname
end

function decode(sockfd)
    local error, msgbuf, msglen, msgname = decodebuf(sockfd)
    if error then
        return error
    end
    local msg = pbc.msgnew(msgname)
    pbc.parse_from_buf(msg, msgbuf, msglen)
    log('[PB][RECV] msgname(%s)', msgname)
    return nil, msgname, msg
end

function flush(sockfd)
    local rptr = Sendbuf.get_read_ptr(sockfd)
    local datalen = Sendbuf.datalen(sockfd)
    if datalen < 0 then
        return 0
    end
    local sent = Socket.send(sockfd, rptr, datalen)
    --log('sockfd(%d) datalen(%d) sent(%d)', sockfd, datalen, sent)
    if sent < 0 then
        return 0
    end
    Sendbuf.skip_read_ptr(sockfd, sent)
    return sent
end


function send(sockfd, msg)
    local seq = 0
    local msgname = pbc.msgname(msg)
    local msglen = pbc.bytesize(msg)
    local plen = 2 + 2 + 2 + string.len(msgname) + msglen
    local buf = Sendbuf.alloc(sockfd, plen)
    local arfd = Ar.create(buf, plen)
    Ar.writeint16(arfd, plen)
    Ar.writeint16(arfd, seq)
    Ar.writelstr(arfd, msgname)
    pbc.serialize(msg, Ar.getptr(arfd))
    Sendbuf.flush(sockfd, buf, plen)
    Ar.free(arfd)
    Port.add_write_event(sockfd)
    log('[PB][SEND] plen(%d) msgname(%s) msglen(%d)', plen, msgname, msglen)
    return plen
end
