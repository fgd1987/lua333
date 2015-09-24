#include <netdb.h>
#include <execinfo.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <ifaddrs.h>


static int lfork(lua_State *L){
	int ir = fork();
    lua_pushinteger(L, ir);
    return 1;
}

static luaL_Reg lua_lib[] ={
    {"fork", lfork},
    {NULL, NULL}
};

int luaopen_os(lua_State *L){
	luaL_register(L, "Date", (luaL_Reg*)lua_lib);
	return 1;
}

