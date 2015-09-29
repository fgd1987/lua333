module('MySqlPort', package.seeall)


logger = Logger.get_logger('MySqlPort')

mysql_connections = {}
mysql_count = 0


function main()
    for pos, mysql_cfg in pairs(Config.db_srv.mysql_srv) do
        mysql_count = mysql_count + 1
        local mysql = MySql.create()
        if mysql:connect(mysql_cfg.host, mysql_cfg.dbname, mysql_cfg.user, mysql_cfg.password) then
            logger:log(string.format('success connect to mysql %s', mysql_cfg.host))
            mysql_connections[pos] = mysql
        else
            logger:error(string.format('connect to %s %s %s %s fail', mysql_cfg.host, mysql_cfg.user, mysql_cfg.password, mysql_cfg.dbname))
            error('connect mysql fail')
        end
    end
end


function select_connection(uid)
    local pos = math.fmod(uid, mysql_count) + 1
    local mysql = mysql_connections[pos]
    while not mysql:ping() do
        logger:error('ping mysql fail')
    end
    return mysql
end

