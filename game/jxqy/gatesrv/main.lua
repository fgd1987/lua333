module('Main', package.seeall)

function main()
end

--配置
config= {
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


