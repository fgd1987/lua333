--配置

local gatesrv_mod = {
    'mod/strproto',
    'mod/pbproto',
    'mod/framesrv',
    'framework/distsrv/gatesrv/clientsrv',
    'framework/distsrv/gatesrv/gameclient',
}

local gamesrv_mod = {
    'mod/strproto',
    'mod/srvproto',
    'mod/postproto',
    'mod/callproto',
    'mod/pbproto',
    'mod/framesrv',
    --'framework/distsrv/gatesrv/clientsrv',
    'framework/distsrv/gatesrv/gameclient',

    'framework/distsrv/gamesrv/gatesrv',
    --'framework/distsrv/gamesrv/globalclient',

    --'framework/distsrv/globalsrv/globalsrv',
    --'framework/distsrv/globalsrv/gamesrv',

--    'framework/distsrv/globalsrv/globalclient',
--    'framework/distsrv/gamesrv/dbclient',
--    'framework/distsrv/gamesrv/gatesrv',
}

local globalsrv_mod = {
    'mod/strproto',
    'mod/pbproto',
    'mod/framesrv',
    'framework/distsrv/dbsrv/dbsrv',
}

Config = {
    mysql_conf = {
        dbname = {
            host = '127.0.0.1', user = 'root', password = '333',
            tables = {
                xx = {binary = true, file = 'xxx'},
            },
        },
    },
    engine_path = '/Volumes/NO NAME/lua333',
    --模块搜索路径
    mod_path = {},
    --lua文件搜索路径
    package = {},

    --服务器列表
    srvlist = {
        {srvid = 1001, srvname = 'gatesrv1', mod = gatesrv_mod},
        {srvid = 2001, srvname = 'globalsrv1', mod = globalsrv_mod},
        {srvid = 1,    srvname = 'gamesrv1', mod = gamesrv_mod},
        {srvid = 2,    srvname = 'gamesrv1', mod = gamesrv_mod},
        {srvid = 3,    srvname = 'gamesrv1', mod = gamesrv_mod},
    },

    --各个模块的配置
    clientsrv = {
        host = '0.0.0.0', port = 3331,
        check_interval = 120,
        protodir = 'proto',
        maxsock = 65536,
    },

    globalsrv = {
        host = '0.0.0.0', port = 3333,
    },

    gatesrv = {
        host = '0.0.0.0', port = 3332,
    },

    gamesrv = {
        host = '0.0.0.0', port = 3334,
    },

    initdb = {
        mysql_conf = {
            actordb = {
                host = '127.0.0.1', user = 'root', password = '333',
                tables = {
                    user = {binary = true, file = 'db.user'}
                },
            },
        },
    },

    dbsrv = {
        delay_write = false,
        dbprotodir = 'dbproto',
        redis_conf = {
            {dbname = 'xxx', host = '127.0.0.1', port = 333, password = '333'},
        },
        mysql_conf = {
            {dbname = 'xxx', host = '127.0.0.1', port = 333, password = '333'},
        },
        table_conf = {
            user = {binary = true, file = 'db.user'},
        },
    },

    globalclient = {
        globalsrv_list = {
            {alias = 'CentersrvSock', srvid = 1, host = '127.0.0.1', port = 3333},
        },
    },

    gameclient = {
        gamesrv_list = {
            {srvid = 1, host = '127.0.0.1', port = 3332},
        },
    },
}

