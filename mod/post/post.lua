module('Post', package.seeall)

--[[
    1.可靠的消息传输
    2.以实体ID为目标参数
    3.投递失败会有通知
    4.投递失败会保存数据库
    5.实体ID并不等于玩家ID，也可以代表系统ID, 如玩家向VIP系统充值的消息

    --投递消息
    --entityid 实体id
    --success_callback 成功回调
    --fail_callback 失败回调
    local wait = POST(entityid).Vip.add_money(uid, 333)
    local xx, xx, xx, = yield wait

--]]

function set_default_stub(stubid)
    default_stubid = stubid
end

function call_method(stubid, entityid, mod_str, method_str, param)
    --更新路由表
    if stubid then Route.add_route(stubid, entityid) end
    --查找路由表
    if not stubid then stubid = Route.find(entityid) end
    --到中心服
    if not stubid then sutid = xxx end
    if not stubid then
        logger:log('stubid not found entityid(%d)', entityid)
    end

end

--@src_stubid 源根
--@src_entityid 源实体
--@dst_entityid 目标实体
function recv_method(src_stubid, src_entityid, dst_entityid, mod_str, method_str, param)
    --更新路由表
    if src_entityid then
        Route.add(src_stubid, src_entityid)
    end
    local src_entity = Entity.find(dst_entityid)
    --如果不在这个服,转发到中心服
    if not src_entity then
        call_method(default_stubid, entityid, mod_str, method_str, param)
    else
        --调用这个实体的方法
        local mod = _G[mod_str]
        if not mod then
            logger:log('mod not found mod(%s)', method)
            return
        end
        local method = mod[method_str]
        if not method then
            return
            logger:log('method not found method(%s)', method)
        end
        method(src_entityid, param)
    end
end

_G['RPC'] = function(entityid, src_entityid)
    local obj = {entityid, src_entityid}
    return obj
end

