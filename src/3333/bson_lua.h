#ifndef _BSON_LUA_H_
#define _BSON_LUA_H_
#include "bson.h"

extern "C"{

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}



int bson_init(lua_State *L);

char *bson_encode(lua_State *L);
int bson_decode(lua_State* L, char *str);


#endif
