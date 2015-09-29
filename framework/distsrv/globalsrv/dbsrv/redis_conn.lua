module('Dbsrv', package.seeall)

redis_connections = {}



function check_redis_connections()
    for index, conf in pairs(Config.dbsrv.redis_srv) do
        local redis = Redis.connect(conf.host, conf.port, conf.password, conf.dbname)
        if not redis then
            logger:log(string.format('fail connect redis host(%s) port(%d) dbname(%s)', conf.host, conf.port, conf.dbname))
            redis_connections[index] = nil
        end
            logger:log(string.format('success connect redis host(%s) port(%d) dbname(%s)', conf.host, conf.port, conf.dbname))
            redis_connections[index] = redis
        end
    end
end

function select_redis(uid)
    local index = math.fmod(uid, #redis_connections) 
    local redis = redis_connections[index + 1]
    return redis
end

function ping(redis)
    local reply = Redis.command(redis, 'PING')
    if not reply or reply.value ~= 'PONG' then
        while not redis:reconnect() do
            logger:error('reconnect fail')
        end
    end
end

