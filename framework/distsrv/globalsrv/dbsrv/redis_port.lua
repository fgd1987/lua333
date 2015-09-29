module('RedisPort', package.seeall)

redis_session = {}

function main()
    for pos, redis_cfg in pairs(Config.dbsrv.redis_srv) do
        local redis = Redis.connect(redis_cfg.host, redis_cfg.port, redis_cfg.password, redis_cfg.dbname)
        if not redis then
            logger:log(string.format('fail to connect redis host(%s) port(%d) dbname(%s)', redis_cfg.host, redis_cfg.port, redis_cfg.dbname))
            error('connect redis fail')
        end
        logger:log(string.format('success connect to redis host(%s) port(%d) dbname(%s)', redis_cfg.host, redis_cfg.port, redis_cfg.dbname))
        redis_count = redis_count + 1
        redis_manager[pos] = redis
        redis_cfg_manager[redis] = redis_cfg
    end
end

function select_connection(uid)
    local pos = math.fmod(uid, redis_count) + 1
    local redis = redis_manager[pos]
    ping(redis)
    return redis
end

function ping(redis)
    local reply = Redis.command(redis, 'PING')
    if not reply or reply.value ~= 'PONG' then
        while not redis:reconnect() do
            logger:error('reconnect fail')
            os.sleep(1)
        end
    end
end

