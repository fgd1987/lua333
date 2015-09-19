#ifndef _L_MYSQL_H_
#define _L_MYSQL_H_

#include "mysql.h"
extern "C"{
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

typedef struct MySql{
    MYSQL *conn;
}MySql;



int lmysql_init(lua_State *L);
#endif
