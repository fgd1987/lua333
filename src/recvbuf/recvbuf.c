#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#define int16 short
#define int32 int
#define int64 long long

typedef struct RBuf{
	int buf_len;
    int rptr;
    int wptr;
	char *buf;
}RBuf;

//1M * 4 * 4 = 16M
#define MAX_SOCKET 1048576

static RBuf rbufs[MAX_SOCKET];

static int mem_total = 0;

#define fd2rbuf(sockfd)  (&rbufs[sockfd])

#define assert_sockfd(sockfd) if(sockfd <= 0 || sockfd >= MAX_SOCKET){\
                        printf("sockfd(%d) is invalid", sockfd);\
                        return 0;\
                     }
static int get_buf(int sockfd, void *vbuf, int buf_len){
    RBuf *self = fd2rbuf(sockfd);
    char *buf = self->buf + self->rptr;
    memcpy(vbuf, buf, buf_len);
    return buf_len;
}

static int read_buf(int sockfd, void *vbuf, int buf_len){
    RBuf *self = fd2rbuf(sockfd);
    char *buf = self->buf + self->rptr;
    memcpy(vbuf, buf, buf_len);
    self->rptr += buf_len;
    return buf_len;
}


static int lfree(lua_State *L){
    return 0;
}

static int lbuf_remain(lua_State *L){
    int sockfd;
    sockfd = (int)lua_tointeger(L, 1);
    assert_sockfd(sockfd);
    RBuf *self = fd2rbuf(sockfd);
    return self->buf_len - self->wptr;
}

static int lget_len(lua_State *L){
    int sockfd;
    sockfd = (int)lua_tointeger(L, 1);
    assert_sockfd(sockfd);
    RBuf *self = fd2rbuf(sockfd);
    return self->buf_len;
}

static int lget_size(lua_State *L){
    int sockfd;
    sockfd = (int)lua_tointeger(L, 1);
    assert_sockfd(sockfd);
    RBuf *self = fd2rbuf(sockfd);
    int size = self->wptr - self->rptr;
    lua_pushinteger(L, size);
    return 1;
}

static int lget_rptr(lua_State *L){
    int sockfd;
    sockfd = (int)lua_tointeger(L, 1);
    assert_sockfd(sockfd);
    RBuf *self = fd2rbuf(sockfd);
    char *ptr = self->buf + self->rptr;
    lua_pushlightuserdata(L, ptr);
    return 1;
}

static int lget_wptr(lua_State *L){
    int sockfd;
    sockfd = (int)lua_tointeger(L, 1);
    RBuf *self = fd2rbuf(sockfd);
    char *ptr = self->buf + self->wptr;
    lua_pushlightuserdata(L, ptr);
    return 1;
}

static int lrskip(lua_State *L){
    int sockfd;
    int len;
    sockfd = (int)lua_tointeger(L, 1);
    len = (int)lua_tointeger(L, 2);
    RBuf *self = fd2rbuf(sockfd);
    self->rptr += len;
    lua_pushinteger(L, len);
    return 1;
}

static int lwskip(lua_State *L){
    int sockfd;
    int len;
    sockfd = (int)lua_tointeger(L, 1);
    len = (int)lua_tointeger(L, 2);
    RBuf *self = fd2rbuf(sockfd);
    self->wptr += len;
    lua_pushinteger(L, len);
    return 1;
}
 
static int lbuf2line(lua_State *L){
    int sockfd;
    sockfd = (int)lua_tointeger(L, 1);
    RBuf *self = fd2rbuf(sockfd);
    if(self->rptr == 0) {
        return 0;
    }
    int len = self->wptr - self->rptr;
    memmove(self->buf, self->buf + self->rptr, len);
    self->rptr = 0;
    self->wptr = len;
    return 0;
}

static int lread_int16(lua_State *L){
    int sockfd;
    sockfd = (int)lua_tointeger(L, 1);
    int16 val = 0;
    read_buf(sockfd, (char *)&val, sizeof(int16));
    lua_pushinteger(L, val);
    return 1;
}

static int lread_int32(lua_State *L){
    int sockfd;
    sockfd = (int)lua_tointeger(L, 1);
    int32 val = 0;
    read_buf(sockfd, (char *)&val, sizeof(int32));
    lua_pushinteger(L, val);
    return 1;
}


static int lget_int16(lua_State *L){
    int sockfd;
    sockfd = (int)lua_tointeger(L, 1);
    int16 val = 0;
    get_buf(sockfd, (char *)&val, sizeof(int16));
    lua_pushinteger(L, val);
    return 1;
}

static int lget_int32(lua_State *L){
    int sockfd;
    sockfd = (int)lua_tointeger(L, 1);
    int32 val = 0;
    get_buf(sockfd, (char *)&val, sizeof(int32));
    lua_pushinteger(L, val);
    return 1;
}

static int lcreate(lua_State *L){
    int sockfd;
    int size;
    sockfd = (int)lua_tointeger(L, 1);
    size = (int)lua_tointeger(L, 2);
    RBuf *self = fd2rbuf(sockfd);
    if(self->buf_len < size){
        if(self->buf != NULL){
            free(self->buf);
            self->buf_len = 0;
            self->buf = NULL;
            mem_total -= self->buf_len;
        }
        self->buf = (char *)malloc(size);
        if(self->buf == NULL){
            printf("malloc fail");
            return 0;
        }
        self->buf_len = size;
        mem_total += size;
    }
    self->rptr = 0;
    self->wptr = 0;
    return 0;
}

static int ltest(lua_State *L){
    printf("test\n");
    return 0;
}

static luaL_Reg lua_lib[] ={
    {"test", ltest},
    {"create", lcreate},
    {"free", lfree},
    {"read_int16", lread_int16},
    {"read_int32", lread_int32},
    {"get_int16", lget_int16},
    {"get_int32", lget_int32},
    {"buf2line", lbuf2line},
    {"wskip", lwskip},
    {"rskip", lrskip},
    {"get_wptr", lget_wptr},
    {"get_rptr", lget_rptr},
    {"get_size", lget_size},
    {"get_len", lget_len},
    {"buf_remain", lbuf_remain},
    {NULL, NULL}
};

int luaopen_recvbuf(lua_State *L){
    luaL_newlib(L, lua_lib);
	return 1;
}
