module('RpcProto', package.seeall)

--[[

    不可靠的消息传输，会丢包   
   
    --调用接口
    POST(sockfd).Login.logout(uid)

    --消息分发
    RPCProto.dispatch(sockfd)
    RPC.dispatch(sockfd)

    --刷新发送缓存
    RPCProto.flush(sockfd)
    RPC.flush(sockfd)

    --发送的底层接口
    RPCProto.send(sockfd, mod_name, func_name, ...)
    RPC.send(sockfd, mod_name, func_name, ...)
--]]

local xx = {
    __index = function(t, mod_name)
        t.mod_name 
    end
}

local tt

function POST(sockfd)
    tt.sockfd = sockfd
    return tt
end



INT_TYPE = 1
STR_TYPE = 2
PROTOBUF_TYPE = 3
JSON_TYPE = 4
BSON_TYPE = 5

--消息分发
function dispatch(sockfd)
    local buf = RecvBuff.rbuf(sockfd)
    local buflen RecvBuff.rlen(sockfd)
    local real_recv = Socket.recv(sockfd, buf, buflen)
    RecvBuff.rskip(sockfd)
    while true do
        if RecvBuff.buflen(sockfd) < 4 then
            break
        end
        local plen = RecvBuff.get_int(sockfd)
        if RecvBuff.buflen(sockfd) < plen then
            break
        end
        local mod_name = RecvBuff.read_str(sockfd)
        local func_name = RecvBuff.read_str(sockfd)
        local param_count = RecvBuff.read_short(sockfd)
        local args = {}
        for i = 1, param_count do
            local type = RecvBuff.read_short(sockfd)
            if type == INT_TYPE then
                table.insert(args, RecvBuff.read_int(sockfd))
            elseif type == STR_TYPE then
                table.insert(args, RecvBuff.read_string(sockfd))
            elseif type == PROTOBUF_TYPE then
                local msgname = RecvBuff.read_str(sockfd)
                local paramlen = RecvBuff.read_int(sockfd)
                local paramptr = RecvBuff.get_ptr(sockfd)
                local msg = Protobuf.decode(msgname, paramptr, paramlen)
                table.insert(args, msg)
            elseif type == JSON_TYPE then
                local paramlen = RecvBuff.read_int(sockfd)
                local paramptr = RecvBuff.get_ptr(sockfd)
                local json = Json.decode(paramptr, paramlen)
                table.insert(args, json)
            elseif type == BSON_TYPE then
                local paramlen = RecvBuff.read_int(sockfd)
                local paramptr = RecvBuff.get_ptr(sockfd)
                local bson = Bson.decode(paramptr, paramlen)
                table.insert(args, bson)
            end
        end
        RecvBuff.rskip(sockfd, plen)
        local mod = _G[mod_name]
        lcoal func = mod[func_name]
        if func then
            func(sockfd, unpack(args))
        end
    end
end

function flush(sockfd)
    local rptr = SendBuff.rptr()
    local rlen = SendBuff.rlen()
    local sent = Socket.send(sockfd, rptr, rlen)
    SendBuff.rskip(sockfd, sent)
end

function send(sockfd, mod_name, func_name, ...)
    local msgname = msg:msgname()
    local buflen = msg:buflen()
    local buf = SendBuff.alloc(sockfd, buflen)
    Pbc.write(msg, buf, buflen)
    SendBuff.append(sockfd, buf, buflen)
end



