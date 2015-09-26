#include <netdb.h>
#include <execinfo.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <fcntl.h>
#include <unistd.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

static int lsetnonblock(lua_State *L){
    int sockfd;
    int error;
    sockfd = (int)lua_tonumber(L, 1);
    error = fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFD, 0) | O_NONBLOCK);
    lua_pushinteger(L, 1);
    return error;
}

static int lclose(lua_State *L){
    int sockfd;
    int error;
    sockfd = (int)lua_tonumber(L, 1);
    error = close(sockfd);
    lua_pushinteger(L, error);
    return 1;
}

static int ltest(lua_State *L){
    printf("test\n");
    return 0;
}

static int lsocket(lua_State *L){
    int sockfd;
    int domain;//AF_INET
    int type;//SOCK_STREAM
    int protocol;//0
    domain = (int)lua_tointeger(L, 1);
    type = (int)lua_tointeger(L, 2);
    protocol = (int)lua_tointeger(L, 3);
    sockfd = socket(domain, type, protocol);
    lua_pushinteger(L, sockfd);
    return 1;
}

static int llisten(lua_State *L){
    int sockfd;
    char *host;
    unsigned short port;
    int error;
    sockfd = (int)lua_tointeger(L, 1);
    host = (char *)lua_tostring(L, 2);
    port = (unsigned short)lua_tointeger(L, 3);
    error = 0;
    lua_pushinteger(L, error);
    return 1;
}

static int lsend(lua_State *L){
    size_t len;
    int error;
    char *str;
    int sockfd;
    sockfd = (int)lua_tonumber(L, 1);
    str = (char *)lua_tolstring(L, 2, &len);
    error = send(sockfd, str, len, 0);
    lua_pushinteger(L, error);
    return 1;
}

static luaL_Reg lua_lib[] ={
    {"test", ltest},
    {"socket", lsocket},
    {"listen", llisten},
    //{"recv", lrecv},
    {"send", lsend},
    {"close", lclose},
    {"setnonblock", lsetnonblock},
    {NULL, NULL}
};

int luaopen_socket(lua_State *L){
	luaL_register(L, "Socket", lua_lib);
	return 1;
}

