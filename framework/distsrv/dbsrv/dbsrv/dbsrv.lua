module('Dbsrv', package.seeall)

descriptors = {}

function main()
    Pbc.import_dir(Config.dbsrv.dbprotodir)
    --构建descriptor
    for table_name, table_conf in pairs(Config.dbsrv.table_conf) do
        if table_conf.binary == false then
            Dbsrv.descriptors[table_name] = pbc.get_descriptor(table_conf.file)
        end
    end
end

function update()

end

--功能:从MYSQL读取数据
--@uid 玩家id
--@table_name 表名
--@return false.出错 nil.没有数据 string.返回数据
local function select_from_mysql(uid, table_name)
    local conn = select_mysql_connection(uid)
    if not conn then
        return false
    end
    local sql_str = string.format('SELECT * FROM %s where uid=%d', table_name, uid)
    local cursor = Mysql.select(conn, sql_str)
    if not cursor then
        return false
    end
    local result = cursor[1] and cursor[1] or nil
    if not cursor then
        Log.error(TAG, 'select %d.%s fail', uid, table_name)
        return false
    elseif not result then
        return nil
    else
        local descriptor = Dbsrv.descriptors[table_name]
        if descriptor then
            local pb_file = Config.dbsrv.table_conf[table_name].file
            local msg = pbc.msgnew(pb_file)
            for field_name, field in pairs(descriptor) do
                if field.type == 'message' then
                    local filed_msg = msg[field_name]
                    if not pbc.parse_from_string(filed_msg, result[field_name]) then
                        logerr('%d.%s.%s parse fail', uid, table_name, field_name)
                        return false
                    end
                else
                    msg[field_name] = result[field_name]
                end
            end
            local table_bin = pbc.tostring(msg)
            if not table_bin then
                Log.error(TAG, 'load from mysql %d.%s tostring fail', uid, table_name)
                return false
            end
            log('load from mysql %d.%s(%d)', uid, table_name, string.len(table_bin))
            return table_bin
        else
            local table_bin = result.table_bin
            log('load from mysql %d.%s(%d)', uid, table_name, string.len(table_bin))
            return table_bin
        end
    end
end

function reply(sockfd, msg)

end

--功能:取玩家表
--@sockfd
function GET(sockfd, uid, callback, ...)
    local table_array = {...}
    local msg_array = {}
    local hmget_str = string.format('HMGET %d ', uid)
    for _, table_name in pbpairs(tables) do
        hmget_str = hmget_str..' '..table_name..' '
    end
    --从redis拉数据
    local redis = select_redis_connection(uid)
    local redis_reply = Redis.command(redis, hmget_str)
    if not redis_reply then
        Log.error(TAG, 'HMGET fail command(%s)', hmget_str, reply_reply.value)
        POST(sockfd, callback, uid, 1)
        return
    end
    if redis_reply.type ~= 'array' then
        Log.errors(TAG, 'HMGET fail command(%s) error(%s)', hmget_str, redis_reply.value)
        POST(sockfd, callback, uid, 1)
        return
    end
    local total_size = 0
    local mysql_tables = {}
    local load_from_mysql = false
    for idx, v in pairs(redis_reply.value) do
        local table_name = table_array[idx]
        if v.type == 'string' then
            table.insert(msg_array, v.value)
            Log.log(TAG, 'load from redis %d.%s(%d)', uid, user_table.table_name, string.len(v.value))
            total_size = total_size + string.len(v.value)
        else
            load_from_mysql = true
            table.insert(mysql_tables, table_name)
        end
    end

    if not load_from_mysql then
        --logger:db(140001, uid, 0, msg.table_name:count(), total_size, 0, 0, 0)
        --EXPIRE
        redis:command(string.format('EXPIRE %d %d', uid, Config.db_srv.expire_sec))
        POST(sockfd, callback, uid, 0, unpack(msg_array))
        return
    end
    --从mysql拉数据
    local mset_args = {}
    for _, table_name in pairs(mysql_tables) do
        local table_name = user_table.table_name
        local table_bin = select_from_mysql(uid, table_name)
        if table_bin == false then
            POST(sockfd, callback, uid, 0))
            return
        elseif table_bin == nil then
            user_table.table_status = 1
            user_table.table_bin = ''
        else
            table.insert(mset_args, table_name)
            table.insert(mset_args, table_bin)
            total_size = total_size + string.len(table_bin)
        end
    end
    --logger:db(140001, uid, 0, msg.table_name:count(), total_size, 0, 0, 0)
    --存到redis
    Redis.hmset(redis, uid, unpack(mset_args))
    Redis.command(redis, string.format('EXPIRE %d %d', uid, Config.dbsrv.expire_sec))
    POST(sockfd, callback, uid, 0, unpack(msg_array))
end

--保存到redis
function save_to_redis(sockfd, msg)
    local uid = msg.uid
    local mset_args = {}
    local save_list = ''..uid
    local table_statuses = msg.table_statuses
    for _, user_table in pbpairs(msg.tables) do
        local table_name = user_table.table_name
        local table_bin = user_table.table_bin
        table.insert(mset_args, table_name)
        table.insert(mset_args, table_bin)
        save_list = save_list..' '..table_name
    end
    --logger:db(140002, uid, 0, msg.tables:count(), total_size, 0, 0, 0)
    local redis = select_redis_connection(uid)
    --存redis
    local redis_reply = Redis.hmset(redis, uid, unpack(mset_args))
    if not redis_reply or (redis_reply.type == 'status' and redis_reply.value ~= 'OK') then
        Log.error(TAG, 'cache %d fail', uid)
        return
    end
    --EXPIRE
    local redis_reply = Redis.command(redis, string.format('EXPIRE %d %d', uid, Config.dbsrv.expire_sec))
    if not redis_reply or redis_reply.value ~= 1 then
        Log.error(TAG, string.format('expire %d fail', uid))
        return
    end
    --存SAVE LIST
    if savelist ~= '' then
        local redis_reply = Redis.lpush(redis, 'savelist', savelist)
        if not redis_reply or redis_reply.type ~= 'integer' then
            Log.error(TAG, 'queue %d FAIL', uid)
            return
        end
    end
    return true
end

function save_to_mysql(sockfd, msg)
    local uid = msg.uid
    local total_size = 0
    local conn = select_mysql_connection(uid)
    if not conn then
        Log.error(TAG, 'select mysql connection fail')
        return
    end
    local timenow = os.time()
    for _, user_table in pbpairs(msg.tables) do
        local table_name = user_table.table_name
        local table_bin = user_table.table_bin
        --是否要展开这个表
        local descriptor = descriptors[table_name]
        if not descriptor then
            local sql_str = string.format("replace into %s (uid, table_bin, update_time) VALUES (%d, '%s', %d)", table_name, uid, Mysql.escape(conn, table_bin), timenow)
            if not Mysql.command(conn, sql_str) then
                Log.error(TAG, '%s replace fail', table_name)
                return
            end
            Log.log(TAG, 'save %d.%s(%d) success', uid, table_name, string.len(table_bin))
        else
            local file_path = Config.dbsrv.table_conf[table_name].file
            local msg = pbc.msgnew(file_path)
            pbc.parse_from_string(msg, table_bin)
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
                    sql_str = sql_str..'\''..Mysql.e(conn, value)..'\','
                elseif field.type == 'message' then
                    sql_str = sql_str..'\''..Mysql.e(conn, value:tostring())..'\','
                end
            end
            sql_str = sql_str..timenow..')'
            Log.log(TAG, sql_str)
            if not Mysql.command(conn, sql_str) then
                Log.error(TAG, '%s expand fail', table_name)
                return
            end
            Log.log(TAG, 'save %d.%s(%d) success', uid, table_name, string.len(table_bin))
        end
    end
    return true
end

--功能:存玩家表
--@sockfd
--@msg db_srv.SET
function MSG_SET(sockfd, msg)
    local uid = msg.uid
    local total_size = 0
    local table_statuses = msg.table_statuses
    for _, user_table in pbpairs(msg.tables) do
        local table_name = user_table.table_name
        local table_bin = user_table.table_bin
        local table_status = table_statuses:add()
        table_status.table_name = table_name
        table_status.status = ''
        total_size = total_size + string.len(table_bin)
    end
    Log:log(TAG, 'set table_count(%d) total_size(%d)', msg.tables:count(), total_size)
    if msg.tables:count() <= 0 then
        reply(sockfd, msg)
        return
    end
    if not save_to_redis(sockfd, msg) then
        reply(sockfd, msg)
        return
    end
    if not Config.dbsrv.delay_write then
        if not save_to_mysql(sockfd, msg) then
            reply(sockfd, msg)
            return
        end
    end
    for _, table_status in pbpairs(msg.table_statuses) do
        table_status.status = 'OK'
    end
    reply(sockfd, msg)
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
        Log.log(TAG, 'load from redis %d.%s', uid, table_name)
        local str = string.format('hget %d %s', uid, table_name)
        local redis = select_redis_connection(uid)
        local redis_reply = Redis.command(redis, str)
        if redis_reply and redis_reply.type == 'string' then
            user_table.table_bin = redis_reply.value
            Log.log(TAG, 'load from redis %d.%s(%d) success', uid, table_name, string.len(redis_reply.value))
        elseif redis_reply and redis_reply.type == 'null' then
            Log.log(TAG, 'load from mysql %d.%s', uid, table_name)
            local table_bin = select_from_mysql(uid, table_name)
            if not table_bin then
                user_table.table_bin = ''
            else
                user_table.table_bin = table_bin
                Redis.hset(redis, uid, table_name, table_bin)
                Redis.command(redis, string.format('EXPIRE %d %d', uid, Config.db_srv.expire_sec))
            end
        else
            user_table.table_bin = ''
        end
    end
    reply(sockfd, msg)
end

--功能:修改K,V属性(只能修改展开的表)
function MSG_KVSET(sockfd, msg)
    local uid = msg.uid
    local mysql = select_mysql_connection(uid)
    if not mysql then
        Log.error(TAG, '%d.%s mysql disconnect', uid, msg.table_name)
        for _, kv in pbpairs(msg.kvs) do
            Log.error(TAG, '%s %s', kv.key, kv.value)
        end
        return
    end
    local sql = string.format('UPDATE %s set ', msg.table_name)    
    for _, kv in pbpairs(msg.kvs) do
        sql = sql..string.format("%s='%s', ", kv.key, kv.value)
    end
    sql = sql..string.format(' update_time=%d ', os.time())
    sql = sql..string.format(' WHERE uid=%d', uid)
    if not Mysql.command(mysql, sql) then
        Log.error(TAG, 'kvset fail %s', sql)
    end
    reply(sockfd, msg)
end
