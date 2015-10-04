module('Dbsrv', package.seeall)

mysql_table = mysql_table or {}


function check_mysql_connections()
    --关闭
    for index, info in pairs(mysql_table) do
        local conf = Config.dbsrv.mysqls_conf[index]
        if not conf or conf.host ~= info.host or conf.dbname ~= info.dbname or conf.user ~= info.user then
            Log.log(TAG, 'close mysql connection index(%d) host(%s) dbname(%s)', index, conf.host, conf.dbname)
            Mysql.close(info.conn)
            mysql_table[index] = nil
        end
    end
    --连接
    for index, conf in pairs(Config.dbsrv.mysql_conf) do
        local conn = Mysql.connect(conf.host, conf.dbname, conf.user, conf.password)
        if conn then
            Log.log(TAG, 'success connect mysql index(%d) host(%s) dbname(%s) user(%s)', conf.host, conf.dbname, conf.user)
            mysql_connections[index] = {conn = conn, host = conf.host, dbname = conf.dbname, user = conf.user}
        else
            Log.log(TAG, 'fail connect mysql index(%d) host(%s) dbname(%s) user(%s)', index, conf.host, conf.dbname, conf.user, conf.password)
            mysql_connections[index] = nil
        end
    end
end

function select_mysql_connection(uid)
    local index = math.fmod(uid, #mysql_connections)
    local info = mysql_connections[index + 1]
    return info and info.conn or nil
end

function ping_mysql(conn)
    local reply = Mysql.ping(conn)
    if not reply then
        for index, info in pairs(mysql_table) do
            if info.conn == conn then
                Mysql.close(info.conn)
                msyql_table[index] = nil
                Log.log(TAG, 'ping fail, close mysql connection index(%d) host(%s) dbname(%s)', index, conf.host, conf.dbname)
                break
            end
        end
    end
end
