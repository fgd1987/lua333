local config = {
    mysql_conf = {
        actordb = {
            dbproto_dir = 'dbproto',
            host = '127.0.0.1', user = 'root', password = '',
            tables = {
                user = {file = 'db.user.User'}
            },
        },
    },
}
return config
