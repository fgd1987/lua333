module('Dbsrv', package.seeall)


mysql_connections = {}


function check_mysql_connections()
    for index, conf in pairs(Config.dbsrv.mysql_srv) do
        local mysql = Mysql.connect(conf.host, conf.dbname, conf.user, conf.password)
        if mysql then
            Log.log(TAG, 'success connect mysql %s %s %s %s', conf.host, conf.user, conf.password, conf.dbname)
            mysql_connections[index] = mysql
        else
            Log.log(TAG, 'fail connect %s %s %s %s', conf.host, conf.user, conf.password, conf.dbname)
            mysql_connections[index] = nil
        end
    end
end

function select_mysql(uid)
    local index = math.fmod(uid, #mysql_connections)
    local mysql = mysql_connections[index + 1]
    while not Mysql.ping(mysql) do
        Log.log(TAG, 'ping mysql fail')
    end
    return mysql
end
