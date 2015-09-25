#include "ar.h"
#include <stdlib.h>
#include <string.h>

#define int8 char
#define int16 short
#define int32 int
#define int64 long long
/*
 *   local arfd = Ar.create(buf, buflen)
 *   local uid = Ar.read_int(arfd)
 *   local uname = Ar.read_str(arfd)
 */

#define MAX_AR 64
static ar_t s_ars[MAX_AR];
#define fd2ar(fd)  (&s_ars[fd])

int ar_create(char *buf, int buf_len){
    int fd = 0;
    int i;
    for(i = 1; i < MAX_AR; i++)
    {
        if(s_ars[i].used == 0)
        {   
            fd = i;
            s_ars[i].used = 1;
            s_ars[i].buf = buf;
            s_ars[i].buf_len = buf_len;
            s_ars[i].rptr = 0;
            break;
        }
    }
    return fd;
}

void ar_free(int fd){
    ar_t *self = fd2ar(fd);
    self->used = 0;
}

int ar_remain(int fd){
    ar_t *self = fd2ar(fd);
    return self->buf_len - self->rptr;
}

char *ar_ptr(int fd){
    ar_t *self = fd2ar(fd);
    return self->buf + self->rptr;
}

static int ar_write(int fd, void *vbuf, int buf_len){
    ar_t *self = fd2ar(fd);
    if (self->buf_len - self->rptr < buf_len) {
        return 0;
    }
    char *buf = self->buf + self->rptr;
    memcpy(buf, vbuf, buf_len);
    self->rptr += buf_len;
    return buf_len;
}

static int ltest(lua_State *L){
    printf("test\n");
    return 0;
}
static int lcreate(lua_State *L){
	if (lua_gettop(L) == 2 && lua_isuserdata(L, 1) && lua_isnumber(L, 2)){
        char *buf = (char *)lua_touserdata(L, 1);
        int buf_len = (int)lua_tointeger(L, 2);
        int fd = ar_create(buf, buf_len);
        lua_pushinteger(L, fd);
        return 1;
    }
    return 0;
}

static int lwrite_int8(lua_State *L){
	if (lua_gettop(L) == 2 && lua_isnumber(L, 1) && lua_isnumber(L, 2)){
        int fd = (int)lua_tointeger(L, 1);
        int8 val = (int8)lua_tointeger(L, 2);
        ar_write(fd, (char *)&val, sizeof(int8));
        lua_pushinteger(L, val); 
        return 1;
	}
    return 0;
}

static int lwrite_int16(lua_State *L){
	if (lua_gettop(L) == 2 && lua_isnumber(L, 1) && lua_isnumber(L, 2)){
        int fd = (int)lua_tointeger(L, 1);
        int16 val = (int16)lua_tointeger(L, 2);
        ar_write(fd, (char *)&val, sizeof(int16));
        lua_pushinteger(L, val); 
        return 1;
	}
    return 0;
}

static int lwrite_int32(lua_State *L){
	if (lua_gettop(L) == 2 && lua_isnumber(L, 1) && lua_isnumber(L, 2)){
        int fd = (int)lua_tointeger(L, 1);
        int32 val = (int32)lua_tointeger(L, 2);
        ar_write(fd, (char *)&val, sizeof(int32));
        lua_pushinteger(L, val); 
        return 1;
	}
    return 0;
}

static int ar_read(int fd, void *vbuf, int buf_len){
    ar_t *self = fd2ar(fd);
    char *buf = self->buf + self->rptr;
    memcpy(vbuf, buf, buf_len);
    self->rptr += buf_len;
    return buf_len;
}

static int lread_int8(lua_State *L){
	if (lua_gettop(L) == 1 && lua_isnumber(L, 1)){
        int fd = (int)lua_tointeger(L, 1);
        int8 val = 0;
        ar_read(fd, (char *)&val, sizeof(int8));
        lua_pushinteger(L, val); 
        return 1;
	}
    return 0;
}

static int lread_int16(lua_State *L){
	if (lua_gettop(L) == 1 && lua_isnumber(L, 1)){
        int fd = (int)lua_tointeger(L, 1);
        int16 val = 0;
        ar_read(fd, (char *)&val, sizeof(int16));
        lua_pushinteger(L, val); 
        return 1;
	}
    return 0;
}

static int lread_int32(lua_State *L){
	if (lua_gettop(L) == 1 && lua_isnumber(L, 1)){
        int fd = (int)lua_tointeger(L, 1);
        int32 val = 0;
        ar_read(fd, (char *)&val, sizeof(int32));
        lua_pushinteger(L, val); 
        return 1;
	}
    return 0;
}

static luaL_Reg lua_lib[] = {
    {"create",      lcreate},
    {"test",        ltest},
    {"read_int8",   lread_int8},
    {"read_int16",  lread_int16},
    {"read_int32",  lread_int32},
    //{"read_str",  lread_str},

    {"write_int8",  lwrite_int8},
    {"write_int16", lwrite_int16},
    {"write_int32", lwrite_int32},
    //{"write_str", lwrite_str},

   // {"seek", lseek},
    {0, 0}
};

int luaopen_ar(lua_State *L){
	luaL_register(L, "Ar", lua_lib);
    return 1;
}
