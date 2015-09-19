#ifndef _AR_H_
#define _AR_H_

extern "C"{

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

typedef struct ar_t{
    char *buf;
    int buf_len;
    int rptr;
    int used;
}ar_t;

int ar_init(lua_State *L);


char *ar_ptr(int fd);
int ar_read_int(int fd);
int ar_read_short(int fd);
int ar_read(int fd, void *vbuf, int buf_len);
int ar_remain(int fd);

int ar_new(char *buf, int buf_len);
void ar_free(int fd);

#endif
