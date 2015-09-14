module('System', package.seeall)


function rpc_ping(entityid, actor, param1, param2)
    RPC(entityid).System.ping(param1, param2)
end



