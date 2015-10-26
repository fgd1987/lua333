module('Main', package.seeall)

function main()
    print('globalsrv main')
    --[[
    require('globalsrv')
    require('dbproto')
    local playerdata = dbproto.PlayerData:new()
    print('playerdata', playerdata)
    print('playerdata.uid', playerdata.uid)
    Item.add_item(playerdata, 1, 2)
    Item.del_item(playerdata, 1, 2)
    print('playerdata.uid', playerdata.uid)
    --]]
    Asyncsrv.mainloop()
end

config = {
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
            {dbname = 'dbproto', host = '127.0.0.1', user = 'root', port = 3306, password = ''},
        },
        --结构表和对象的映射
        table_conf = {
            user = {class = 'dbproto.UserData'},
        },
    },
    {'framework/distsrv/globalsrv/gamesrv',
        host = '0.0.0.0', port = 3334,
    },
    {'framework/distsrv/globalsrv/globalsrv',
        host = '0.0.0.0', port = 3333,
    },
    {'framework/distsrv/globalsrv/centersrv/login'},
    {'game/jxqy/globalsrv/guild'},
    {'game/jxqy/globalsrv/item'},
}


