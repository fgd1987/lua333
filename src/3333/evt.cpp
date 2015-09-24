   
#include "evt.h"


static aeEventLoop *loop;

int evt_stop(){
    aeStop(loop);
    return 1;
}

static int RunOnce(lua_State *L){
    aeOnce(loop);
    return 1;
}

static luaL_Reg lua_lib[] = {
    {"RunOnce", RunOnce},
    {NULL, NULL}
};


int evt_init(lua_State *L){
    luaL_register(L, "Evt", lua_lib);
    loop = aeCreateEventLoop(8192);
    if(loop == NULL){
        return 0;
    }
    return 1;
}


int evt_run(){
    LOG_LOG("event_dispatch begin");
    aeMain(loop);
    LOG_LOG("event_dispatch end");
    return 1;
}


struct aeEventLoop *evt_loop(){
    return loop;
}
