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
#include <strings.h>
#include <errno.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#define LOG_ERROR printf

static int lsetnonblock(lua_State *L){
    int sockfd;
    int error;
    sockfd = (int)lua_tonumber(L, 1);
    error = fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFD, 0) | O_NONBLOCK);
    lua_pushinteger(L, error);
    return 1;
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

static int laccept(lua_State *L){
    int listenfd;
    int sockfd;
    listenfd = (int)lua_tointeger(L, 1);
	struct sockaddr_in addr;	
	socklen_t addrlen = sizeof(addr);	
    sockfd = accept(listenfd, (struct sockaddr*)&addr, &addrlen);
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
	struct sockaddr_in addr;
	bzero((void*)&addr, sizeof(addr));	
	addr.sin_family = AF_INET;	
	addr.sin_addr.s_addr = INADDR_ANY;	
	addr.sin_port = htons(port);	
    int reuse = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
	error = bind(sockfd,(struct sockaddr *)&addr,sizeof(addr));
	if(error < 0){		
        LOG_ERROR("fail to bind %d %d %s", port, errno, strerror(errno));
        lua_pushinteger(L, error);
		return 1;	
    }
	error = listen(sockfd, 5);	
	if(error < 0){		
        LOG_ERROR("fail to listen %d %s", errno, strerror(errno));
        lua_pushinteger(L, error);
		return 1;	
	}
    lua_pushinteger(L, error);
    return 1;
}

static int lrecv(lua_State *L){
    int error;
    int sockfd;
    char *buf;
    int buflen;
    sockfd = (int)lua_tointeger(L, 1);
    buf = (char *)lua_touserdata(L, 2);
    buflen = (int)lua_tointeger(L, 3);
    error = recv(sockfd, buf, buflen, 0);
    lua_pushinteger(L, error);
    return 1;
}

static int lsend(lua_State *L){
    int error;
    int sockfd;
    char *buf;
    size_t len;
    sockfd = (int)lua_tointeger(L, 1);
    buf = (char *)lua_touserdata(L, 2);
    len = (int)lua_tointeger(L, 3);
    error = send(sockfd, buf, len, 0);
    lua_pushinteger(L, error);
    return 1;
}

static luaL_Reg lua_lib[] ={
    {"test", ltest},
    {"socket", lsocket},
    {"listen", llisten},
    {"accept", laccept},
    {"recv", lrecv},
    {"send", lsend},
    {"close", lclose},
    {"setnonblock", lsetnonblock},
    {NULL, NULL}
};

int luaopen_socket(lua_State *L){
	luaL_register(L, "Socket", lua_lib);

    lua_pushstring(L, "AF_INET");
    lua_pushinteger(L, AF_INET);
    lua_settable(L, -3);

    lua_pushstring(L, "SOCK_STREAM");
    lua_pushinteger(L, SOCK_STREAM);
    lua_settable(L, -3);

	return 1;
}

