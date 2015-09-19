#ifndef _REDIS_H_
#define _REDIS_H_

#include "hiredis.h"
#include "async.h"
#include "os.h"
extern "C"{
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}


typedef struct AsyncRedis{
    struct redisAsyncContext *c;
    char cb_connect[64]; 
    char cb_disconnect[64]; 
    char ip[32];
    unsigned short port;
}AsyncRedis;

typedef struct Redis{
    struct redisContext *c;
    char ip[32];
    unsigned short port;
}Redis;

int redis_init(lua_State *L);
#endif
