/**
*
*
*
**/
#include "config.h"
#include "luas.h"
#include "evt.h"
#include "ar.h"
#include "port.h"
#include "timer.h"
#include "sys.h"
#include "pbc.h"
#include "pb_port.h"
#include "raw_port.h"
#include "rmi_port.h"
#include "redis.h"
#include "pb_socket.h"
#include "lualua.h"
#include "lmysql.h"
#include "json.h"
#include "bbuf.h"
#include "rbuf.h"
#include "bson_encode.h"
#include "queue.h"
#include <execinfo.h>

static void sig_int(int b){
    LOG_LOG("sig_int");
    //evt_stop();
    lua_getglobal(L, "_exit");
    if(!lua_isnil(L, -1)){
        lua_pcall(L, 0, 0, 0);
    }
    luas_close();
    pbc_exit();   
    exit(0);
}
static void sig_usr2(int b){
    LOG_LOG("sig_usr2");
    lua_getglobal(L, "sig_usr2");
    if(!lua_isnil(L, -1)){
        lua_pcall(L, 0, 0, 0);
    }
}


static void sig_usr1(int b){
    LOG_LOG("sig_usr1");
    lua_getglobal(L, "sig_usr1");
    if(!lua_isnil(L, -1)){
        lua_pcall(L, 0, 0, 0);
    }
}

static int lua_gc_loop(int msgid, void *userdata){
    lua_gc(L, LUA_GCSTEP, 100);
    return 1;
}
/**
    整个系统的初始化
**/
int system_init(int argc, char **argv)
{
	//初始化Lua
	luas_init();
    lua_State *L = luas_state();

    lua_gc(L, LUA_GCSTOP, 0);
	//初始化Event
	evt_init(L);
    //初始化timer
    timer_init(L);
    //初始化log
    log_init(L);
    //初始化port
    port_init(L);     
    bbuf_init();
    rbuf_init();
    //初始化ar
    ar_init(L);
    //初始化pb库
    pbc_init(L);
    pb_port_init(L);
    raw_port_init(L);
    rmi_port_init(L);
    lmysql_init(L);
    redis_init(L);
    json_init(L);
    pb_socket_init(L);
    bson_regist(L);
    queue_init(L);

    sys_init(L, argc, argv);


    //signal(SIGSEGV,  sig_core);
    //signal(SIGBUS,  sig_core);
    //signal(SIGFPE,  sig_core);
    //signal(SIGILL,  sig_core);
    

    signal(SIGINT,   sig_int);

    signal(SIGHUP, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);
    signal(SIGTERM, SIG_IGN);
    signal(SIGHUP, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    //google protobuf出错时候会出这个
    signal(SIGABRT, SIG_IGN);
 
    signal(SIGUSR1, sig_usr1);
    signal(SIGUSR2, sig_usr2);

    timer_add_callback(lua_gc_loop, 1000, NULL);
    return 1;
}

int system_run(int argc, char **argv)
{
    int ir = luamain(argc, argv);
    if(ir != EXIT_FAILURE){
        evt_run();
        return 1;
    }else{
        return 0;
    }
}

int system_destory()
{
    lua_getglobal(L, "_exit");
    if(!lua_isnil(L, -1)){
        lua_pcall(L, 0, 0, 0);
    }
    LOG_LOG("system exit");
    //luas_close();
    //pbc_exit();    
    return 1;
}

int main(int argc, char **argv)
{

    //系统初始化
    system_init(argc, argv);

    //运行
    system_run(argc, argv);

    //结束
    system_destory();
    LOG_LOG("(%d)exit...\n", getpid()); 
    return 1;
}
