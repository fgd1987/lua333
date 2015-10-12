module('Dbsrv', package.seeall)

descriptors = {}

function _init()
    Pbc.import_dir(_CONF.dbproto_dir)
    --构建descriptor
    for table_name, table_conf in pairs(_CONF.table_conf) do
        if not table_conf.binary then
            Dbsrv.descriptors[table_name] = pbc.get_descriptor(table_conf.file)
        end
    end
    check_redis_connections()
    check_mysql_connections()
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
        logerr('select %d.%s fail', uid, table_name)
        return false
    end
    local pb_file = _CONF.table_conf[table_name].file
    local msg = pbc.msgnew(pb_file)
    if not result then
        return msg, ''
    end
    local descriptor = Dbsrv.descriptors[table_name]
    if descriptor then
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
            logerr('load from mysql %d.%s tostring fail', uid, table_name)
            return false
        end
        log('load from mysql %d.%s(%d)', uid, table_name, string.len(table_bin))
        return msg, table_bin
    else
        local table_bin = result.table_bin
        pbc.parse_from_string(msg, table_bin)
        log('load from mysql %d.%s(%d)', uid, table_name, string.len(table_bin))
        return msg, table_bin
    end
end

function get_from_redis(srvid, uid, callback, ...)
    local table_array = {...}
    local msg_array = {}
    local hmget_str = string.format('HMGET %d ', uid)
    for _, table_name in pairs(table_array) do
        hmget_str = hmget_str..' '..table_name..' '
    end
    --从redis拉数据
    local conn = select_redis_connection(uid)
    if not conn then
        return
    end
    local redis_reply = Redis.command(conn, hmget_str)
    if not redis_reply then
        logerr('HMGET fail command(%s)', hmget_str, reply_reply.value)
        return
    end
    if redis_reply.type ~= 'array' then
        logerr('HMGET fail command(%s) error(%s)', hmget_str, redis_reply.value)
        return
    end
    local total_size = 0
    if redis_reply and redis_reply.type == 'array' then
        for idx, v in pairs(redis_reply.value) do
            local table_name = table_array[idx]
            if v.type == 'string' then
                local msg = pbc.msgnew(_CONF.table_conf[table_name].file)
                pbc.parse_from_string(msg, v.value)
                table.insert(msg_array, msg)
                log('load from redis %d.%s(%d)', uid, table_name, string.len(v.value))
                total_size = total_size + string.len(v.value)
            else
                return
            end
        end
    end
    Redis.command(conn, string.format('EXPIRE %d %d', uid, _CONF.expire_sec))
    POST(srvid, callback, uid, 1, unpack(msg_array))
    return true
end

--功能:取玩家表
--@srvid
function GET(srvid, uid, callback, ...)
    if _CONF.redis_conf then
        if get_from_redis(srvid, uid, callback, ...) then
            return
        end
    end
    local total_size = 0
    local table_array = {...}
    local msg_array = {}
    local mset_args = {}
    for _, table_name in pairs(table_array) do
        local msg, table_bin = select_from_mysql(uid, table_name)
        if not msg then
            POST(srvid, callback, uid, 0)
            return
        else
            table.insert(msg_array, msg)
            table.insert(mset_args, table_name)
            table.insert(mset_args, table_bin)
            total_size = total_size + string.len(table_bin)
        end
    end
    log('total_size(%d)', total_size)
    --存到redis
    if _CONF.redis_conf then
        local conn = select_redis_connection(uid)
        if conn then
            Redis.hmset(conn, uid, unpack(mset_args))
            Redis.command(conn, string.format('EXPIRE %d %d', uid, _CONF.expire_sec))
        end
    end
    POST(srvid, callback, uid, 1, unpack(msg_array))
end

--保存到redis
function save_to_redis(srvid, uid, callback, ...)
    local args = {...}
    local mset_args = {}
    local savelist = ''..uid
    for i = 1, #args, 2 do
        local table_name = args[i]
        local msg = args[i + 1]
        local table_bin = pbc.tostring(msg)
        table.insert(mset_args, table_name)
        table.insert(mset_args, table_bin)
        savelist = savelist..' '..table_name
    end
    local conn = select_redis_connection(uid)
    --存redis
    local redis_reply = Redis.hmset(conn, uid, unpack(mset_args))
    if not redis_reply or (redis_reply.type == 'status' and redis_reply.value ~= 'OK') then
        logerr('cache %d fail', uid)
        return
    end
    --EXPIRE
    local redis_reply = Redis.command(conn, string.format('EXPIRE %d %d', uid, _CONF.expire_sec))
    if not redis_reply or redis_reply.value ~= 1 then
        logerr('expire %d fail', uid)
        return
    end
    --存SAVE LIST
    if savelist ~= '' then
        local redis_reply = Redis.lpush(conn, 'savelist', savelist)
        if not redis_reply or redis_reply.type ~= 'integer' then
            logerr('queue %d FAIL', uid)
            return
        end
    end
    return true
end

function save_to_mysql(srvid, uid, callback, ...)
    local total_size = 0
    local args = {...}
    local conn = select_mysql_connection(uid)
    if not conn then
        logerr('select mysql connection fail')
        return
    end
    local timenow = os.time()
    for i = 1, #args, 2 do
        local table_name = args[i]
        local msg = args[i + 1]
        --是否要展开这个表
        local descriptor = descriptors[table_name]
        if not descriptor then
            local table_bin = pbc.tostring(msg)
            local sql_str = string.format("replace into %s (uid, table_bin, update_time) VALUES (%d, '%s', %d)", table_name, uid, Mysql.escape(conn, table_bin), timenow)
            if not Mysql.command(conn, sql_str) then
                logerr('%s replace fail', table_name)
                return
            end
            log('save %d.%s(%d) success', uid, table_name, string.len(table_bin))
        else
            local file_path = _CONF.table_conf[table_name].file
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
                    sql_str = sql_str..'\''..Mysql.escape(conn, value)..'\','
                elseif field.type == 'message' then
                    sql_str = sql_str..'\''..Mysql.escape(conn, value:tostring())..'\','
                end
            end
            sql_str = sql_str..timenow..')'
            log(sql_str)
            if not Mysql.command(conn, sql_str) then
                logerr('%s expand fail', table_name)
                return
            end
            log('save %d.%s(%d) success', uid, table_name, pbc.bytesize(msg))
        end
    end
    return true
end

--功能:存玩家表
--@srvid
--@msg db_srv.SET
function SET(srvid, uid, callback, ...)
    if not save_to_redis(srvid, uid, callback, ...) then
        --写缓存失败，马上写数据库
        if not save_to_mysql(srvid, uid, callback, ...) then
            POST(srvid, callback, uid, 0)
        end
    end
    if not _CONF.delay_write then
        if not save_to_mysql(srvid, uid, callback, ...) then
            POST(srvid, callback, uid, 0)
            return
        end
    end
    POST(srvid, callback, uid, 1)
end


--功能:取玩家表
function MSG_MGET(srvid, msg)
    local uid = msg.uid
    local table_name = msg.table_name
    local tables = msg.tables
    local uids = msg.uids
    for _, uid in pbpairs(uids) do
        local user_table = tables:add()
        user_table.uid = uid
        user_table.table_bin = ''
        log('load from redis %d.%s', uid, table_name)
        local str = string.format('hget %d %s', uid, table_name)
        local redis = select_redis_connection(uid)
        local redis_reply = Redis.command(redis, str)
        if redis_reply and redis_reply.type == 'string' then
            user_table.table_bin = redis_reply.value
            log('load from redis %d.%s(%d) success', uid, table_name, string.len(redis_reply.value))
        elseif redis_reply and redis_reply.type == 'null' then
            log('load from mysql %d.%s', uid, table_name)
            local table_bin = select_from_mysql(uid, table_name)
            if not table_bin then
                user_table.table_bin = ''
            else
                user_table.table_bin = table_bin
                Redis.hset(redis, uid, table_name, table_bin)
                Redis.command(redis, string.format('EXPIRE %d %d', uid, _CONF.expire_sec))
            end
        else
            user_table.table_bin = ''
        end
    end
    reply(srvid, msg)
end

--功能:修改K,V属性(只能修改展开的表)
function MSG_KVSET(srvid, msg)
    local uid = msg.uid
    local mysql = select_mysql_connection(uid)
    if not mysql then
        logerr('%d.%s mysql disconnect', uid, msg.table_name)
        for _, kv in pbpairs(msg.kvs) do
            logerr('%s %s', kv.key, kv.value)
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
        logerr('kvset fail %s', sql)
    end
    reply(srvid, msg)
end
