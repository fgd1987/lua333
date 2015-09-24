module('Call', package.seeall)

--[[
    必须在一个协议里面调用这个接口    
    local co = CALL(sockfd).VipSystem.dec_money(uid, 333)
    local result = yield co
    if result then
        Item.add_item(player, av, 333)
    end
--]]


callid_acc = callid_acc or 1

--回调表
--{[callid] = co}
callback_table = callback_table or {}

MSG_CALL = 1
MSG_RETURN = 2

INT_TYPE = 1
STR_TYPE = 2
PROTOBUF_TYPE = 3
JSON_TYPE = 4
BSON_TYPE = 5

--调用接口
function call(sockfd, mod_name, func_name, ...)
    local co = coroutine.running()
    local callid = callid_acc 
    callid_acc = callid_acc + 1
    callback_table[callid] = co
    --打包消息
end

function encode_args(sockfd, ...)

end

--解包参数
function decode_args(sockfd)
    local arg_count = RecvBuf.read_uint8(sockfd)
    local args = {}
    for i = 1, arg_count do
        local type = RecvBuf.read_uint8(sockfd)
        if type == INT_TYPE then
            table.insert(args, RecvBuf.read_int64(sockfd))
        elseif type == STR_TYPE then
            table.insert(args, RecvBuf.read_string(sockfd))
        elseif type == PROTOBUF_TYPE then
            local msgname = RecvBuf.read_str(sockfd)
            local paramlen = RecvBuf.read_int(sockfd)
            local paramptr = RecvBuf.get_ptr(sockfd)
            local msg = Protobuf.decode(msgname, paramptr, paramlen)
            table.insert(args, msg)
        elseif type == JSON_TYPE then
            local paramlen = RecvBuf.read_int(sockfd)
            local paramptr = RecvBuf.get_ptr(sockfd)
            local json = Json.decode(paramptr, paramlen)
            table.insert(args, json)
        elseif type == BSON_TYPE then
            local paramlen = RecvBuf.read_int(sockfd)
            local paramptr = RecvBuf.get_ptr(sockfd)
            local bson = Bson.decode(paramptr, paramlen)
            table.insert(args, bson)
        end
    end
    return args
end

--消息分发
function dispatch(sockfd)
    local buf = RecvBuf.rbuf(sockfd)
    local buflen RecvBuf.rlen(sockfd)
    local real_recv = Socket.recv(sockfd, buf, buflen)
    RecvBuf.rskip(sockfd)
    while true do
        if RecvBuf.buflen(sockfd) < 4 then
            break
        end
        local plen = RecvBuf.get_int(sockfd)
        if RecvBuf.buflen(sockfd) < plen then
            break
        end
        --消息类型
        local msg_type = RecvBuf.read_int8(sockfd)
        if msg_type == MSG_RETURN then
            local callid = RecvBuf.read_int64(sockfd)
            local args = decode_args(sockdf)
            --唤醒协程
            local co = callback_table[callid]
            resume(co, unpack(args)
        elseif msg_type == MSG_CALL then
            local callid = RecvBuf.read_int64(sockfd)
            local mod_name = RecvBuf.read_str(sockfd)
            local func_name = RecvBuf.read_str(sockfd)
            local mod = _G[mod_name]
            lcoal func = mod[func_name]
            if func then
                local return_args = select(func(sockfd, unpack(args)))
                callback(sockfd, callid, return_args)
            else
                callback(sockfd, callid, nil)
            end
        end
        RecvBuf.rskip(sockfd, plen)
    end
end

function select(...)
    return arg
end

function calc_args_len(sockfd, ...)
    local total_len = 0
    for _, value in ipairs(arg) do
        if type(value) == type(0) then
            return 8
        elseif type(value) == type('') then
            return 2 + string.len(value)
        elseif type(value) == type(nil) then
            return 0
        elseif type(value) == type({}) then
            local json_str = 
        elseif type(value) == 'userdata' then
            --TODO
            --这里可以查询userdata的类型，暂时统一认为是protobuf,
        end
    end
    --参数类型各占一个字节
    return total_len + arg.n
end

--回复对方
function callback(sockfd, callid, ...)
    local arg_len = calc_args_len(sockfd, unpack(arg))
    local plen = 4 + 1 + 8 + arg_len
    SendBuf.alloc(sockfd, plen)
    SendBuf.write_int32(sockfd, plen)
    SendBuf.write_int8(sockfd, MSG_RETURN)
    SendBuf.write_int64(sockfd, callid)
    encode_args(sockfd, unpack(arg))
end
