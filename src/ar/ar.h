#ifndef _AR_H_
#define _AR_H_

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

typedef struct ar_t{
    char *buf;
    int buf_len;
    int rptr;
    int used;
}ar_t;

#endif
