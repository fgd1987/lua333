
#ifndef _SYSCALL_H_
#define _SYSCALL_H_
extern "C"{

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}


int sys_init(lua_State *L, int argc, char **argv);



#endif
