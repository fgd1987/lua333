#include <netdb.h>
#include <execinfo.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

static int lmsectime(lua_State *L){
    struct timeval tv; 
    struct timezone tz; 
    gettimeofday(&tv,&tz);
    long long msec = tv.tv_sec * 1000 + tv.tv_usec / 1000;
    lua_pushnumber(L, msec);
    return 1;
}

static int lgettimeofday(lua_State *L){
    struct timeval tv; 
    struct timezone tz; 
    gettimeofday(&tv,&tz);
    lua_pushinteger(L,tv.tv_sec);
    lua_pushinteger(L,tv.tv_usec);
    return 2;
}

static int ltime(lua_State *L){
    int t = time(NULL);
    lua_pushinteger(L, t);
    return 1;
}

static int lstrftime(lua_State *L){
    if(lua_gettop(L) == 2 && lua_isstring(L, 1) && lua_isnumber(L, 2)){
        const char *format = (const char *)lua_tostring(L, 1);
        time_t time = (time_t)lua_tonumber(L, 2);
        struct tm *tm;
        tm = localtime(&time);
        char str[32];
        if(strftime(str, sizeof(str), format, tm)){
        }
        lua_pushstring(L, str);
        return 1;
    }
    return 0;
}

static luaL_Reg lua_lib[] ={
    {"gettimeofday", lgettimeofday},
    {"time", ltime},
    {"msectime", lmsectime},
    {"strftime", lstrftime},
    {NULL, NULL}
};

int luaopen_date(lua_State *L){
	luaL_register(L, "Date", lua_lib);
	return 1;
}

