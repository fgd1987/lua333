--配置

local gatesrv_mod = {
    'mod/strproto',
    'mod/pbproto',
    'mod/framesrv',
    'framework/distsrv/gatesrv/clientsrv',
--    'framework/distsrv/gatesrv/gameclient',
}

local gamesrv_mod = {
    'mod/strproto',
    'mod/pbproto',
    'mod/framesrv',
    'framework/distsrv/gamesrv/gatesrv',
}

local globalsrv_mod = {

}

Config = {
    engine_path = '/Users/lujingwei/lua333',
    --模块搜索路径
    mod_path = {},
    --lua文件搜索路径
    package = {},

    --服务器列表
    srvlist = {
        {srvid = 1001, srvname = 'gatesrv1', mod = gatesrv_mod},
        {srvid = 1,    srvname = 'gamesrv1', mod = gamesrv_mod},
        {srvid = 2,    srvname = 'gamesrv1', mod = gamesrv_mod},
        {srvid = 3,    srvname = 'gamesrv1', mod = gamesrv_mod},
    },

    --各个模块的配置
    clientsrv = {
        host = '0.0.0.0', port = 3333,
        check_interval = 120,
        protodir = 'proto',
        maxsock = 65536,
    },

    gatesrv = {
        host = '0.0.0.0', port = 3333,
    },

    dbsrv = {


    },

    globalclient = {
        globalsrv_list = {
            {srvid = 1, host = '0.0.0.0', port = 333},
            {srvid = 2, host = '0.0.0.0', port = 333},
            {srvid = 3, host = '0.0.0.0', port = 333},
        },
    },

    gameclient = {
        gamesrv_list = {
            {srvid = 1, host = '112.80.248.74', port = 80},
        },
    },
}

