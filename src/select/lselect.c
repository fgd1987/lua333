#include <netdb.h>
#include <unistd.h>
#include <execinfo.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

/*
	Select.add(sockfd)
	Select.remove(sockfd)
	local count = Select.poll()
	for i = 0, count do
		local sockfd, readevent, writeevent = Select.getevent(i)
	end
*/

#define LOG_ERROR printf

#define AE_NONE 0
#define AE_READABLE 1
#define AE_WRITABLE 2

#define MAX_SOCKET 65536

typedef struct FiredEvent {
    int sockfd;
    int mask;
}FiredEvent;

FiredEvent fired_events[MAX_SOCKET];

typedef struct FileEvent {
    int mask;
}FileEvent;

typedef struct EventLoop {
    int maxsockfd;
    FileEvent events[MAX_SOCKET];
    fd_set read_fds;
    fd_set write_fds;
    fd_set _read_fds;
    fd_set _write_fds;
}EventLoop;

static int lcreate(lua_State *L) {
    EventLoop *loop = (EventLoop *)malloc(sizeof(EventLoop)); 
    memset(loop, 0, sizeof(EventLoop));
    lua_pushlightuserdata(L, loop);
    return 1;
}

static int lpoll(lua_State *L) {
    EventLoop *loop = (EventLoop *)lua_touserdata(L, 1);
    if (!loop) {
        lua_pushinteger(L, 0);
        return 1;
    }
    int retval, j, numevents = 0;
    struct timeval tvp;
    memset(&tvp, 0, sizeof(tvp));
    memcpy(&loop->_read_fds, &loop->read_fds, sizeof(fd_set));
    memcpy(&loop->_write_fds, &loop->write_fds, sizeof(fd_set));

    retval = select(loop->maxsockfd + 1, &loop->_read_fds, &loop->_write_fds, NULL, &tvp);
    if (retval > 0) {
        for (j = 0; j <= loop->maxsockfd; j++) {
            int mask = 0;
            FileEvent *fe = &loop->events[j];
            if (fe->mask == AE_NONE) continue;
            if (fe->mask & AE_READABLE && FD_ISSET(j, &loop->_read_fds))
                mask |= AE_READABLE;
            if (fe->mask & AE_WRITABLE && FD_ISSET(j, &loop->_write_fds))
                mask |= AE_WRITABLE;
            if (mask) {
                fired_events[numevents].sockfd = j;
                fired_events[numevents].mask = mask;
                numevents++;
            }
        }
    }
    lua_pushinteger(L, numevents);
    return 1;
}

static int lremove(lua_State *L) {
    EventLoop *loop = (EventLoop *)lua_touserdata(L, 1);
    int sockfd = (int)lua_tointeger(L, 2);
    if (sockfd >= MAX_SOCKET) {
        LOG_ERROR("eeeee");
        return 0;
    }
	FD_CLR(sockfd, &loop->read_fds);
	FD_CLR(sockfd, &loop->write_fds);
    loop->events[sockfd].mask = 0;
    for (int i = sockfd; i >= 0; i--) {
        if (loop->events[i].mask != 0) {
            loop->maxsockfd = i;
            return 0;
        }
    }
    loop->maxsockfd = 0;
	return 0;
}

static int ladd_write_event(lua_State *L) {
    EventLoop *loop;
    int sockfd;
    loop = (EventLoop *)lua_touserdata(L, 1);
    sockfd = (int)lua_tointeger(L, 2);
    if (sockfd >= MAX_SOCKET) {
        LOG_ERROR("");
        return 0;
    }
	FD_SET(sockfd , &loop->write_fds);
    loop->events[sockfd].mask |= AE_WRITABLE;
    if (sockfd > loop->maxsockfd) {
        loop->maxsockfd = sockfd;
    }
	return 0;
}


/*
 *@arg1 loop 
 *@arg2 sockfd
 */
static int ladd_read_event(lua_State *L) {
    EventLoop *loop;
    int sockfd;
    loop = (EventLoop *)lua_touserdata(L, 1);
    sockfd = (int)lua_tointeger(L, 2);
    if (sockfd >= MAX_SOCKET) {
        LOG_ERROR("");
        return 0;
    }
	FD_SET(sockfd , &loop->read_fds);
    loop->events[sockfd].mask |= AE_READABLE;
    if (sockfd > loop->maxsockfd) {
        loop->maxsockfd = sockfd;
    }
	return 0;
}

static int lgetevent(lua_State *L) {
    EventLoop *loop = (EventLoop *)lua_touserdata(L, 1);
    int index = (int)lua_tointeger(L, 2);
    //下标从1开始
    index = index - 1;
    if (index < 0 || index >= MAX_SOCKET) {
        return 0;
    }
    int sockfd = fired_events[index].sockfd;
    int mask = fired_events[index].mask;
    lua_pushinteger(L, sockfd);
    lua_pushboolean(L, mask & AE_READABLE ? 1 : 0);
    lua_pushboolean(L, mask & AE_WRITABLE ? 1 : 0);
    return 3;
}

static int lfree(lua_State *L) {
    EventLoop *loop = (EventLoop *)lua_touserdata(L, 1);
    free(loop);
    return 0;
}

static luaL_Reg lua_lib[] = {
    {"create", lcreate},
    {"free", lfree},
    {"add_read_event", ladd_read_event},
    {"add_write_event", ladd_write_event},
    {"remove", lremove},
    {"poll", lpoll},
    {"getevent", lgetevent},
    {NULL, NULL}
};

int luaopen_select(lua_State *L){
	luaL_register(L, "Select", lua_lib);

    lua_pushstring(L, "READABLE");
    lua_pushinteger(L, AE_READABLE);
    lua_settable(L, -3);

    lua_pushstring(L, "WRITABLE");
    lua_pushinteger(L, AE_WRITABLE);
    lua_settable(L, -3);

	return 1;
}

