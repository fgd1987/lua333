module('Strproto', package.seeall)
--[[
telnet协议，运行时处理一些系统命令, gm之类的
--]]

function dispatch(sockfd)
    --填充recvbuf
    local buf = Recvbuf.getwptr(sockfd)
    local bufremain = Recvbuf.bufremain(sockfd)
    print('bufremain', bufremain)
    local recv = Socket.recv(sockfd, buf, bufremain)
    print('recv', recv)
    if recv == 0 then
        --peer close half connection
        return
    end
    if recv == -1 then
        return
    end
    Recvbuf.wskip(sockfd, recv)
    --读buf
    local pos = Recvbuf.find(sockfd, '\r\n')
    if not pos then
        Log.log(TAG, 'data not enough')
        return
    end
    local str = Recvbuf.readbuf(sockfd, pos)
    print('recv', str)
    Recvbuf.rskip(sockfd, 2)
    Recvbuf.buf2line(sockfd)
    reply(sockfd, str)
end

function reply(sockfd, str)
    local len = string.len(str) + 2
    local buf = Sendbuf.alloc(sockfd, len)
    local arfd = Ar.create(buf, len)
    local write = Ar.writestr(arfd, str)
    Log.log(TAG, 'reply(%d)', write)
    local write = Ar.writestr(arfd, '\r\n')
    Log.log(TAG, 'reply(%d)', write)
    Sendbuf.flush(sockfd, buf, len)
    Ar.free(arfd)
end

function flush(sockfd)
    local buf = Sendbuf.rptr(sockfd)
    local datalen = Sendbuf.datalen(sockfd)
    local sent = Socket.send(sockfd, buf, datalen)
    Log.log(TAG, 'sent(%d)', sent)
    if sent > 0 then
        Sendbuf.rskip(sockfd, sent)
    end
end
