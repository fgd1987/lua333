
#include "timer.h"
#include "evt.h"
#include "luas.h"


static char s_self_timer_callback[64];

static int s_timer_counter = 0;
static int s_min_msec = 0x7fffffff;
//穿越lua的次数
static int s_call_lua_time = 0;

int timer_stat(){
    LOG_STAT("timer(%d) min_msec(%d) call(%d)", s_timer_counter, s_min_msec, s_call_lua_time);
    return 1;
}
int timer_stop(int timerid){
    aeDeleteTimeEvent(evt_loop(), timerid);
    return 1;
}
static int on_cpp_timer(struct aeEventLoop *eventLoop, long long id, void *clientData){
    Timer *timer = (Timer *)clientData;

    int ir = timer->callback(timer->msgid, timer->userdata);
    if(ir == 1)
    {
        return timer->msec;
    }else{
        s_timer_counter--;
        free(timer);
        return AE_NOMORE;
    }
}

int timer_add_callback(timer_callback callback, int msec, void *userdata){
    Timer *timer = (Timer *)malloc(sizeof(Timer));
    if(timer == NULL){
        LOG_ERROR("malloc fail");
        return 0;
    }
    timer->msec = msec;
    timer->userdata = userdata;
    timer->callback = callback;
    s_timer_counter++;
    if(s_min_msec > msec)s_min_msec = msec;
    int timerid = aeCreateTimeEvent(evt_loop(), msec, on_cpp_timer, timer, NULL);
    timer->timerid = timerid;
    return timerid;
}

/*
 * 如果lua返回0则删除timer, 
 * 如果n=0, 则以上次的时间再次触发
 * 如果n>0, 则n毫秒后再次触发
 *
 */
static int on_lua_timer(struct aeEventLoop *eventLoop, long long id, void *clientData){
    LuaTimer *timer = (LuaTimer *)clientData;
    if(timer == NULL){
        LOG_ERROR("timer is null");
        s_timer_counter--;
        return AE_NOMORE;
    }
    if(luas_pushluafunction(timer->callback) == 0){
        LOG_ERROR("push func %s fail", timer->callback);
        s_timer_counter--;
        free(timer);
        return AE_NOMORE;
    }
    LOG_LOG("[TIMER_START] %s", timer->callback);
    s_call_lua_time++;
    if(lua_pcall(L, 0, 1, 0) != 0){
        LOG_ERROR("%s", lua_tostring(L, -1));
        lua_pop(L, lua_gettop(L));
        return timer->msec;
    }
    LOG_LOG("[TIMER_END] %s", timer->callback);

    int ir = (int)lua_tointeger(L, -1);
    if(ir == 1){
        lua_pop(L, lua_gettop(L));
        return timer->msec;
    }else if(ir != 0){
        lua_pop(L, lua_gettop(L));
        return ir;
    }else{
        lua_pop(L, lua_gettop(L));
        s_timer_counter--;
        free(timer);
        return AE_NOMORE;
    }
} 

/*
 * 如果lua返回0则删除timer, 如果n>0, 则n毫秒后再次触发
 *
 */
static int on_lua_self_timer(struct aeEventLoop *eventLoop, long long timerid, void *clientData){
    size_t selfid = (size_t)clientData;
    if(luas_pushluafunction(s_self_timer_callback) == 0){
        LOG_ERROR("push func %s fail", s_self_timer_callback);
        s_timer_counter--;
        return AE_NOMORE;
    }
    lua_pushnumber(L, timerid);
    lua_pushnumber(L, selfid);
    s_call_lua_time++;
    if(lua_pcall(L, 2, 1, 0) != 0){
        LOG_ERROR("%s", lua_tostring(L, -1));
        lua_pop(L, lua_gettop(L));
        s_timer_counter--;
        return AE_NOMORE;
    }
    int ir = (int)lua_tointeger(L, -1);
    lua_pop(L, lua_gettop(L));
    if(ir != 0){
        return ir;
    }else{
        s_timer_counter--;
        return AE_NOMORE;
    }
} 

static int AddTimer(lua_State *L){

    if (lua_gettop(L) == 2 && lua_isnumber(L, 1) && lua_isstring(L, 2)) {
        int msec = (int)lua_tonumber(L, 1);
        const char *funcname = (const char *)lua_tostring(L, 2);
        
        LuaTimer *timer = (LuaTimer *)malloc(sizeof(LuaTimer));

        if(timer == NULL){
            LOG_ERROR("malloc fail");
            return 0;
        }
        timer->msec = msec;
        strcpy(timer->callback, funcname);
        s_timer_counter++;
        if(s_min_msec > msec)s_min_msec = msec;
        int timerid = aeCreateTimeEvent(evt_loop(), msec, on_lua_timer, timer, NULL);
        timer->timerid = timerid;

        lua_pushnumber(L, timerid);
        return 1;
    }
    lua_pushboolean(L, false);
    return 1;
}

static int SetSelfTimerLoop(lua_State *L){
    if (lua_gettop(L) == 1 && lua_isstring(L, 1)) {
        const char *callback   = (const char *)lua_tostring(L, 1);

        strcpy(s_self_timer_callback, callback);
        lua_pushboolean(L, true);
        return 1;
    }
    lua_pushboolean(L, false);
    return 1;
}
static int StopSelfTimer(lua_State *L){

    if (lua_gettop(L) == 1 && lua_isnumber(L, 1)) {
        int timerid = (int)lua_tonumber(L, 1);

        if(aeDeleteTimeEvent(evt_loop(), timerid) == AE_OK){
            s_timer_counter--;
        }
        lua_pushboolean(L, true);
        return 1;
    }
    lua_pushboolean(L, false);
    return 1;
}


static int AddSelfTimer(lua_State *L){

    if (lua_gettop(L) == 2 && lua_isnumber(L, 1) && lua_isnumber(L, 2)) {
        int msec = (int)lua_tonumber(L, 1);
        size_t selfid = (size_t)lua_tonumber(L, 2);
        
        s_timer_counter++;
        if(s_min_msec > msec)s_min_msec = msec;
        int timerid = aeCreateTimeEvent(evt_loop(), msec, on_lua_self_timer, (void *)selfid, NULL);
        lua_pushnumber(L, timerid);
        return 1;
    }
    lua_pushboolean(L, false);
    return 1;
}

static luaL_Reg lua_lib[] = {
    {"AddTimer", AddTimer},
    {"AddSelfTimer", AddSelfTimer},
    {"StopSelfTimer", StopSelfTimer},
    {"SetSelfTimerLoop", SetSelfTimerLoop},
    {NULL, NULL}
};


int timer_init(lua_State *L){
    luaL_register(L, "Timer", lua_lib);

    return 1;
}
