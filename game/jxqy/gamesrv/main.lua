module('Main', package.seeall)

function main()
    print('gamesrv main')
    require('gamesrv')
    import('mod/asyncsrv')
    --[[
    local playerdata = dbproto.PlayerData:new()
    print('playerdata', playerdata)
    print('playerdata.uid', playerdata.uid)
    Item.add_item(playerdata, 1, 2)
    Item.del_item(playerdata, 1, 2)
    print('playerdata.uid', playerdata.uid)
    Pbc.test()
    local msg = Pbc.msgnew('dbproto.PlayerData')
    print(msg:msgname())
    --]]
    Asyncsrv.mainloop()
end

config= {
    {'mod/strproto'},
    {'mod/srvproto'},
    {'mod/postproto'},
    {'mod/callproto'},
    {'mod/pbc'},
    {'framework/distsrv/gamesrv/gatesrv',
        host = '0.0.0.0', port = 3332,
        proto = 'proto',
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
        dbproto = 'dbproto',
        save_interval = 3600,
        playerdata = {'user'},
    },
    {'framework/distsrv/gamesrv/player'},
    {'game/jxqy/gamesrv/item'},
}


