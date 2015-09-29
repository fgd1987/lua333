module('Dbsrv', package.seeall)

descriptors = {}

function main()
    --构建descriptor
    for table_name, table_conf in pairs(Config.table_conf) do
        if table_conf.binary == false then
            Db_srv.descriptors[table_name] = pbc.get_descriptor(table_conf.file)
        end
    end
end

--功能:从MYSQL读取数据
--@uid 玩家id
--@table_name 表名
--@return false.出错 nil.没有数据 string.返回数据
local function select_from_mysql(uid, table_name)
    local mysql = MySqlPort.select_connection(uid)
    if not mysql then
        return false
    end
    local sql_str = string.format('SELECT * FROM %s where uid=%d', table_name, uid)
    local cursor = mysql:select(sql_str)
    if not cursor then
        return false
    end
    local result = cursor[1] and cursor[1] or nil
    if not cursor then
        logger:error(string.format('select %d.%s fail', uid, table_name))
        return false
    elseif not result then
        return nil
    else
        local descriptor = Db_srv.descriptors[table_name]
        if descriptor then
            local pb_file = Config.table_conf[table_name].file
            local msg = pbc.msgnew(pb_file)
            for field_name, field in pairs(descriptor) do
                if field.type == 'message' then
                    local filed_msg = msg[field_name]
                    if not filed_msg:parse_from_string(result[field_name]) then
                        logger:error(string.format('%d.%s.%s parse fail', uid, table_name, field_name))
                        return false
                    end
                else
                    msg[field_name] = result[field_name]
                end
            end
            local table_bin = msg:tostring()
            if not table_bin then
                logger:error(string.format('load from mysql %d.%s tostring fail', uid, table_name))
                return false
            end
            logger:log(string.format('load from mysql %d.%s(%d)', uid, table_name, string.len(table_bin)))
            return table_bin
        else
            local table_bin = result.table_bin
            logger:log(string.format('load from mysql %d.%s(%d)', uid, table_name, string.len(table_bin)))
            return table_bin
        end
    end
end

--功能:取玩家表
--@sockfd
--@msg db_srv.GET
function MSG_GET(sockfd, msg)
    local uid = msg.uid

    local hmget_str = string.format('HMGET %d ', uid)

    for _, table_name in pbpairs(msg.table_name) do
        local user_table = msg.tables:add()
        user_table.table_name = table_name
        user_table.table_bin = ''
        user_table.table_status = 0
        hmget_str = hmget_str..' '..table_name..' '
    end

    --从redis拉数据
    local redis = RedisPort.select_connection(uid)

    local redis_reply = redis:command(hmget_str)
    if not redis_reply then
        logger:error(string.format('HMGET fail command(%s)', hmget_str, reply_reply.value))
        PBPort.post(sockfd, msg)
        return
    end

    if redis_reply.type ~= 'array' then
        logger:error(string.format('HMGET fail command(%s) error(%s)', hmget_str, redis_reply.value))
        PBPort.post(sockfd, msg)
        return
    end

    local total_size = 0
    local mysql_tables = {}
    local load_from_mysql = false
    local idx = 1
    for _, v in pairs(redis_reply.value) do
        local user_table = msg.tables:get(idx)
        if v.type == 'string' then
            user_table.table_bin = v.value
            logger:log(string.format('load from redis %d.%s(%d)', uid, user_table.table_name, string.len(v.value)))
            user_table.table_status = 2
            total_size = total_size + string.len(v.value)
        else
            load_from_mysql = true
            table.insert(mysql_tables, user_table)
        end
        idx = idx + 1
    end

    if not load_from_mysql then
        logger:db(140001, uid, 0, msg.table_name:count(), total_size, 0, 0, 0)
        --EXPIRE
        redis:command(string.format('EXPIRE %d %d', uid, Config.db_srv.expire_sec))
        PBPort.post(sockfd, msg)
        return
    end

    --从mysql拉数据
    local mset_args = {}
    for _, user_table in pairs(mysql_tables) do
        local table_name = user_table.table_name
        local table_bin = select_from_mysql(uid, table_name)
        if table_bin == false then
            user_table.table_status = 0
            user_table.table_bin = ''
        elseif table_bin == nil then
            user_table.table_status = 1
            user_table.table_bin = ''
        else
            user_table.table_status = 3
            user_table.table_bin = table_bin
            table.insert(mset_args, table_name)
            table.insert(mset_args, table_bin)
            total_size = total_size + string.len(table_bin)
        end
    end

    logger:db(140001, uid, 0, msg.table_name:count(), total_size, 0, 0, 0)
    --存到redis
    redis:hmset(uid, unpack(mset_args))
    redis:command(string.format('EXPIRE %d %d', uid, Config.db_srv.expire_sec))

    PBPort.post(sockfd, msg)
end

--功能:存玩家表
--@sockfd
--@msg db_srv.SET
function MSG_SET(sockfd, msg)
    local uid = msg.uid

    local total_size = 0
    local mset_args = {}
    local save_list = ''..uid
    local table_statuses = msg.table_statuses
    for _, user_table in pbpairs(msg.tables) do
        local table_name = user_table.table_name
        local table_bin = user_table.table_bin
        local table_status = table_statuses:add()
        table_status.table_name = table_name
        table_status.status = ''
        table.insert(mset_args, table_name)
        table.insert(mset_args, table_bin)
        save_list = save_list..' '..table_name
        logger:log(string.format('save %d.%s(%d) to redis', uid, table_name, string.len(table_bin)))
        total_size = total_size + string.len(table_bin)
    end
    if msg.tables:count() <= 0 then
        PBPort.post(sockfd, msg)
        return
    end
    logger:db(140002, uid, 0, msg.tables:count(), total_size, 0, 0, 0)

    local redis = RedisPort.select_connection(uid)


    --存redis
    local redis_reply = redis:hmset(uid, unpack(mset_args))

    if not redis_reply or (redis_reply.type == 'status' and redis_reply.value ~= 'OK') then
        logger:error(string.format('cache %d fail', uid))
        PBPort.post(sockfd, msg)
        return
    end

    --EXPIRE
    local redis_reply = redis:command(string.format('EXPIRE %d %d', uid, Config.db_srv.expire_sec))
    if not redis_reply or redis_reply.value ~= 1 then
        logger:error(string.format('expire %d fail', uid))
        PBPort.post(sockfd, msg)
        return
    end
    
    if Config.db_srv.delay_write then
        --存SAVE LIST
        if save_list ~= '' then
            local redis_reply = redis:lpush('save_list', save_list)
            if not redis_reply or redis_reply.type ~= 'integer' then
                logger:error(string.format('queue %d FAIL', uid))
                PBPort.post(sockfd, msg)
                return
            end
        end
    else
        local mysql = MySqlPort.select_connection(uid)
        if not mysql then
            logger:error(string.format('select mysql FAIL', uid))
            PBPort.post(sockfd, msg)
            return
        end
        local timenow = os.time()
        for _, user_table in pbpairs(msg.tables) do
            local table_name = user_table.table_name
            local table_bin = user_table.table_bin
            --是否要展开这个表
            local descriptor = descriptors[table_name]
            if not descriptor then
                local sql_str = string.format("replace into %s (uid, table_bin, update_time) VALUES (%d, '%s', %d)", 
                    table_name, uid, mysql:e(table_bin), timenow)
                if not mysql:command(sql_str) then
                    logger:error(table_name..' replace fail')
                    push_user_table(redis, user_table_str)
                    return 1
                end
                logger:log(uid, string.format('save %d.%s(%d) success', uid, table_name, string.len(table_bin)))
            else
                local file_path = Config.table_conf[table_name].file
                local msg = pbc.msgnew(file_path)
                msg:parse_from_string(table_bin)
                local sql_str = string.format('replace into '..table_name..' (')
                for field_name, field in pairs(descriptor) do
                    local value = msg[field_name]
                    sql_str = sql_str..field_name..','
                end
                sql_str = sql_str..' update_time) values ('
                for field_name, field in pairs(descriptor) do
                    local value = msg[field_name]
                    value = value and value or ''
                    if field.type == 'int' then
                        sql_str = sql_str..value..','
                    elseif field.type == 'string' then
                        sql_str = sql_str..'\''..mysql:e(value)..'\','
                    elseif field.type == 'message' then
                        sql_str = sql_str..'\''..mysql:e(value:tostring())..'\','
                    end
                end
                sql_str = sql_str..timenow..')'
                logger:log(sql_str)
                if not mysql:command(sql_str) then
                    logger:error(table_name..' expand fail')
                    push_user_table(redis, user_table_str)
                    return 1
                end
                logger:log(uid, string.format('save %d.%s(%d) success', uid, table_name, string.len(table_bin)))
            end
        end
    end

    for _, table_status in pbpairs(msg.table_statuses) do
        table_status.status = 'OK'
    end
    PBPort.post(sockfd, msg)
end


--功能:取玩家表
function MSG_MGET(sockfd, msg)
    local uid = msg.uid
    local table_name = msg.table_name

    local tables = msg.tables
    local uids = msg.uids
    for _, uid in pbpairs(uids) do
        local user_table = tables:add()
        user_table.uid = uid
        user_table.table_bin = ''
            
        --load from redis
        logger:log(string.format('load from redis %d.%s', uid, table_name))
        local str = string.format('hget %d %s', uid, table_name)
        local redis = RedisPort.select_connection(uid)
        local redis_reply = redis:command(str)
        if redis_reply and redis_reply.type == 'string' then
            user_table.table_bin = redis_reply.value
            logger:log(string.format('load from redis %d.%s(%d) success', uid, table_name, string.len(redis_reply.value)))
        elseif redis_reply and redis_reply.type == 'null' then
            logger:log(string.format('load from mysql %d.%s', uid, table_name))
            --select MYSQL
            local table_bin = select_from_mysql(uid, table_name)
            if not table_bin then
                user_table.table_bin = ''
            else
                user_table.table_bin = table_bin
                redis:HSet(uid, table_name, table_bin)
                redis:command(string.format('EXPIRE %d %d', uid, Config.db_srv.expire_sec))
            end
        else
            user_table.table_bin = ''
        end
    end
    PBPort.post(sockfd, msg)
end



--功能:修改K,V属性(只能修改展开的表)
function MSG_KVSET(sockfd, msg)
    local uid = msg.uid
    local mysql = MySqlPort.select_connection(uid)
    if not mysql then
        logger:error(string.format('%d.%s mysql disconnect', uid, msg.table_name))
        for _, kv in pbpairs(msg.kvs) do
            logger:error(string.format('%s %s', kv.key, kv.value))
        end
        return
    end
    
    local sql = string.format('UPDATE %s set ', msg.table_name)    
    for _, kv in pbpairs(msg.kvs) do
        sql = sql..string.format("%s='%s', ", kv.key, kv.value)
    end
    sql = sql..string.format(' update_time=%d ', os.time())
    sql = sql..string.format(' WHERE uid=%d', uid)

    if not mysql:command(sql) then
    end
    PBPort.post(sockfd, msg)
end
