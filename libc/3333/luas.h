#ifndef _LUAS_H_
#define _LUAS_H_

#include "os.h"
extern "C"{

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}
#define LUA_TONUMBER(i) lua_tonumber(L, i)
#define LUA_ISNUMBER(i) lua_isnumber(L, i)
#define LUA_TOSTRING(i) lua_tostring(L, i)
#define LUA_ARGNUM lua_gettop(L)
#define LUA_ISSTRING(i) lua_isstring(L, i)
#define LUA_PUSHNUMBER(n) lua_pushnumber(L, n)
#define LUA_PUSHSTRING(s) lua_pushstring(L, s)
#define LUA_RETURN(r) { lua_pushnumber(L, r); return 1; }
#define LUA_RETURNSTR(s) { lua_pushstring(L, s); return 1; }

#define LUA_NEWTABLE lua_newtable(L)
#define LUA_SETTABLE(i) lua_settable(L,i)
#define LUA_ISTABLE(i) lua_istable(L,i)
#define LUA_GETTABLE(i) lua_gettable(L,i)
#define LUA_RMTABLE(i) lua_remove(L,i)
#define LUA_PUSHFUNC(func) lua_pushcfunction(L,func)
#define LUA_REMOVE(n) lua_remove(L, n)

#define LUA_GETGLOBAL(s) lua_getglobal(L,s)
#define LUA_SETGLOBAL(s) lua_setglobal(L,s)
#define LUA_POP(n) lua_pop(L,n)
#define LUA_PUSHNIL lua_pushnil(L)
#define LUA_RETURNNIL { LUA_PUSHNIL; return 1; }

#define LUA_FUNC_INIT(func) {#func, func}

#define luaL_dobuffer(L, buf, len, func) \
    (luaL_loadbuffer(L, buf, len, func) || lua_pcall(L, 0, LUA_MULTRET, 0))

extern lua_State *L;
int luas_init();
int luas_close();


int luas_reg_lib(const char *libname, luaL_Reg *reglist);

int luas_dofile(const char *filepath);

int luas_callfunc(const char *func, const char *format, ...);
int luas_vcallfunc(const char *func, const char *format, va_list ap);


int luas_pushluafunction(const char *func);


lua_State *luas_open();

lua_State *luas_state();
void luas_set_state(lua_State *L);

#endif
