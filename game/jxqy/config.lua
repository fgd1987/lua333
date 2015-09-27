--配置
local engine_path = '/Users/lujingwei/lua333'

--配置也分模块，这样比较清晰
--需要绝对路径, 因为要搜索下面的文件

local gatesrv_mod = {
    'mod/strproto',
    'mod/framesrv',
    'framework/duofu/gatesrv/clientsrv',
}

local gamesrv_mod = {
}

local globalsrv_mod = {

}

Config = {
    engine_path = engine_path,
    --模块搜索路径
    mod_path = {
    },
    --lua文件搜索路径
    package = {
    },

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
    },

    dbsrv_conf = {


    },

    clientsrv_conf = {

    },

    globalclient_conf = {
        globalsrv_list = {
            {srvid = 1, host = '0.0.0.0', port = 333},
            {srvid = 2, host = '0.0.0.0', port = 333},
            {srvid = 3, host = '0.0.0.0', port = 333},
        },
    },

    gameclient = {
        gamesrv_list = {
            {srvid = 1, host = '0.0.0.0', port = 333},
            {srvid = 2, host = '0.0.0.0', port = 333},
            {srvid = 3, host = '0.0.0.0', port = 333},
        },
    },
}

