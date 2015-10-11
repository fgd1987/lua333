--配置
local gatesrv_mod = {
    {'mod/strproto'},
    {'mod/pbproto'},
    {'mod/srvproto'},
    {'mod/callproto'},
    {'mod/postproto'},
    {'mod/asyncsrv'},
    {'framework/distsrv/gatesrv/clientsrv', 
        host = '0.0.0.0', port = 3331,
        tmp_sock_idle_sec = 10,
        packet_count_threshold = 10,
        packet_time_threshold = 10,
        check_interval = 120,
        protodir = 'proto',
        maxsock = 65536,
        route = {},
    },
    {'framework/distsrv/gatesrv/login'},
    {'framework/distsrv/gatesrv/gameclient',
        gamesrv_list = {
            {srvid = 1, host = '127.0.0.1', port = 3332},
        },
    },
}

local gamesrv_mod = {
    {'mod/strproto'},
    {'mod/srvproto'},
    {'mod/postproto'},
    {'mod/callproto'},
    {'mod/pbproto'},
    {'mod/asyncsrv'},
    {'framework/distsrv/gamesrv/gatesrv',
        host = '0.0.0.0', port = 3332,
        protodir = 'proto',
    },
    {'framework/distsrv/gamesrv/globalclient',
        globalsrv_list = {
            {srvid = 2001, host = '127.0.0.1', port = 3334},
        },
        namesrv = {
            CentersrvSockfd = 2001,
            DbsrvSockfd = 2001,
        },
    },
    {'framework/distsrv/gamesrv/login',
        dbproto_dir = 'dbproto',
        save_interval = 3600,
        playerdata = {'user'},
    },
    {'framework/distsrv/gamesrv/player'},
}
local globalsrv_mod = {
    {'mod/timer'},
    {'mod/strproto'},
    {'mod/srvproto'},
    {'mod/postproto'},
    {'mod/callproto'},
    {'mod/asyncsrv'},
    {'framework/distsrv/globalsrv/dbsrv/dbsrv',
        expire_sec = 100,
        delay_write = false,
        dbproto_dir = 'dbproto',
        redis_conf = {
            {dbname = '1', host = '127.0.0.1', port = 6379, password = ''},
        },
        mysql_conf = {
            {dbname = 'actordb', host = '127.0.0.1', user = 'root', port = 3306, password = ''},
        },
        table_conf = {
            user = {file = 'db.user.User'},
        },
    },
    {'framework/distsrv/globalsrv/gamesrv',
        host = '0.0.0.0', port = 3334,
    },
    {'framework/distsrv/globalsrv/globalsrv',
        host = '0.0.0.0', port = 3333,
    },
    {'framework/distsrv/globalsrv/centersrv/login'},
}

Config = {
    --engine_path = '/root/lua333',
    --engine_path = '/Volumes/NO NAME/lua333',
    engine_path = '/Users/lujingwei/Documents/360pan/Project/lua333',
    proc_path = '/Users/lujingwei/Documents/360pan/Project/lua333/game/jxqy/proc',
    --模块搜索路径
    mod_path = {},
    --lua文件搜索路径
    package = {},
    --服务器列表
    srvlist = {
        {srvid = 1001, srvname = 'gatesrv1',    mod = gatesrv_mod},
        {srvid = 2001, srvname = 'globalsrv1',  mod = globalsrv_mod},
        {srvid = 1,    srvname = 'gamesrv1',    mod = gamesrv_mod},
        {srvid = 2,    srvname = 'gamesrv1',    mod = gamesrv_mod},
        {srvid = 3,    srvname = 'gamesrv1',    mod = gamesrv_mod},
    },

    pbtest = {host = '127.0.0.1', port = 3331},

    initdb = {
        mysql_conf = {
            actordb = {
                dbproto_dir = 'dbproto',
                host = '127.0.0.1', user = 'root', password = '',
                tables = {
                    user = {file = 'db.user.User'}
                },
            },
        },
    },
}
