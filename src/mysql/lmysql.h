#ifndef _L_MYSQL_H_
#define _L_MYSQL_H_

#include "mysql.h"
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

typedef struct MySql{
    MYSQL *conn;
}MySql;

#endif
