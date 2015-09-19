#ifndef _TIMER_H_
#define _TIMER_H_

#include "os.h"


typedef int (*timer_callback)(int, void*);

 
typedef struct Timer
{
    timer_callback callback;
    int msec;
    int timerid;
    int msgid;
    void* userdata;
}Timer;


typedef struct LuaTimer
{
    char callback[64];
    int msec;
    int timerid;
    int msgid;
    void* userdata;
}LuaTimer;

int timer_stop(int timerid);
int timer_add_callback(timer_callback callback, int msec, void *userdata);
int timer_init(lua_State *L);

int timer_stat();

#endif
