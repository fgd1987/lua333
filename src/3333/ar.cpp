#include "ar.h"
#include "luas.h"

#define MAX_AR 64

static ar_t s_ars[MAX_AR];

#define fd2ar(fd)  (&s_ars[fd])

#define MAX_LINE_BUF 5

int ar_new(char *buf, int buf_len){
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
int ar_read(int fd, void *vbuf, int buf_len){
    ar_t *self = fd2ar(fd);
    char *buf = self->buf + self->rptr;
    memcpy(vbuf, buf, buf_len);
    self->rptr += buf_len;
    return buf_len;
}

int ar_read_int(int fd){
    int val = 0;
    ar_read(fd, (char *)&val, sizeof(int));
    return val;
}
int ar_read_short(int fd){
    short val = 0;

    ar_read(fd, (char *)&val, sizeof(short));
    return val;
}
static int ReadInt(lua_State *L){

	if (lua_gettop(L) == 1 && lua_isnumber(L, 1)){
        int fd = (int)lua_tonumber(L, 1);
        int ir = ar_read_int(fd);
        
        lua_pushinteger(L, ir); 
        return 1;
	}

    return 0;
}
static int ReadShort(lua_State *L){

	if (lua_gettop(L) == 1 && lua_isnumber(L, 1)){
        int fd = (int)lua_tonumber(L, 1);

        int ir = ar_read_short(fd);
        lua_pushinteger(L, ir); 
        return 1;
	}
    return 0;
}
static luaL_Reg lua_lib[] = {
    {"ReadInt", ReadInt},
    {"ReadShort", ReadShort},
    {NULL, NULL}
};

int ar_init(lua_State *L){
    luaL_register(L, "AR", lua_lib);
    return 1;
}

