#include <string.h>
#include <time.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include "ae.h"

/*
 * 
 *  local loop = Ae.create_event_loop(1024)
 *  Ae.create_file_event(loop, sockfd, Ae.AE_READABLE, on_read)
 *
 */

#define LOG_ERROR printf

//这个东西不会太多的, 回调用的
#define MAX_CLIENT_DATA 1024

typedef struct ClientData {
    lua_State *L;
    char file_proc[64]; //MAX_PATH
    char finalizer_proc[64];
    char time_proc[64];
}ClientData;

int last_client_data_id = -1;

static ClientData client_data_array[MAX_CLIENT_DATA];

static ClientData* get_client_data(int idx) {
    if (idx < 0 || idx >= MAX_CLIENT_DATA) {
        return NULL;
    }
    return &client_data_array[idx];
}

static int get_client_data_idx(lua_State *L, char *file_proc, char *time_proc, char *finalizer_proc) {
    for (int i = 0; i < last_client_data_id; i++) {
        if (client_data_array[i].L == L 
            &&(file_proc != NULL && 0 == strcmp(client_data_array[i].file_proc, file_proc))
            &&(time_proc != NULL && 0 == strcmp(client_data_array[i].time_proc, time_proc))
            &&(finalizer_proc != NULL && 0 == strcmp(client_data_array[i].finalizer_proc, finalizer_proc))
        ) {
            return i;
        }
    }
    int idx = last_client_data_id + 1;
    if (idx >= MAX_CLIENT_DATA) {
        LOG_ERROR("inc the array?");
        return -1;
    }
    client_data_array[idx].L = L;
    if (file_proc != NULL) {
        strcpy(client_data_array[idx].file_proc, file_proc);
    }
    if (time_proc != NULL) {
        strcpy(client_data_array[idx].time_proc, time_proc);
    }
    if (finalizer_proc != NULL) {
        strcpy(client_data_array[idx].finalizer_proc, finalizer_proc);
    }
    return idx;
}


//login.hello_world
//hello_world
int lua_pushluafunction(lua_State *L, const char *func){
    char *start = (char *)func;
    char *class_name = start;
    char *pfunc = start;
    while(*pfunc != 0){
        if(*pfunc == '.' && class_name == start){
            *pfunc = 0;
            lua_getglobal(L, class_name);
            *pfunc = '.';
            if(lua_isnil(L, -1)){
                return 0;
            }
            class_name = pfunc + 1;
        }else if(*pfunc == '.'){
            *pfunc = 0;
            lua_pushstring(L, class_name);
            lua_gettable(L, -2);
            *pfunc = '.';
            if(lua_isnil(L, -1)){
                return 0;
            }
    	    lua_remove(L, -2);//弹出table
            class_name = pfunc + 1;
        }
        pfunc++;
    }
    if(class_name == start){
        lua_getglobal(L, class_name);
        if(lua_isnil(L, -1)){
            return 0;
        }
    }else{
        lua_pushstring(L, class_name);
        lua_gettable(L, -2);
        if(lua_isnil(L, -1)){
            return 0;
        }
        lua_remove(L, -2);//弹出table
    }
    return 1;     

}

static void file_proc(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask) {
    int id = (int)clientData;
    ClientData *client_data = get_client_data(id);
    lua_State *L = client_data->L;
    lua_pushluafunction(L, client_data->file_proc);
    lua_pushnumber(L, sockfd);
    if (lua_pcall(L, 1, 0, 0) != 0){
        LOG_ERROR("error running function %s: %s", client_data->file_proc, lua_tostring(L, -1));
     }
}

static int time_proc(struct aeEventLoop *eventLoop, long long id, void *clientData) {
    int idx = (int)clientData;
    ClientData *client_data = get_client_data(idx);
    lua_State *L = client_data->L;
    lua_pushluafunction(L, client_data->time_proc);
    lua_pushnumber(L, id);
    if (lua_pcall(L, 1, 0, 0) != 0){
        LOG_ERROR("error running function %s: %s", client_data->time_proc, lua_tostring(L, -1));
     }
    return AE_NOMORE;
}

static void finalizer_proc(struct aeEventLoop *eventLoop, void *clientData) {
    int idx = (int)clientData;
    ClientData *client_data = get_client_data(idx);
    lua_State *L = client_data->L;
    lua_pushluafunction(L, client_data->finalizer_proc);
    if (lua_pcall(L, 0, 0, 0) != 0){
        LOG_ERROR("error running function %s: %s", client_data->finalizer_proc, lua_tostring(L, -1));
     }
}
/*  
static void before_sleep_proc(struct aeEventLoop *eventLoop) {
    int idx = (int)clientData;
    ClientData *client_data = get_client_data(idx);
    lua_State *L = client_data->L;
    char *func_name = client_data->func_name;
    lua_pushluafunction(L, func_name);
    if (lua_pcall(L, 0, 0, 0) != 0){
        LOG_ERROR("error running function %s: %s", client_data->sleep_proc, lua_tostring(L, -1));
     }
}
*/


static int lrun_once(lua_State *L) {
    aeEventLoop *event_loop;
    event_loop = (aeEventLoop *)lua_touserdata(L, 1); 
    aeOnce(event_loop);
    return 0;
}

static int lget_api_name(lua_State *L) {
    char *str = aeGetApiName();
    lua_pushstring(L, str);
    return 1;
}

static int lmain(lua_State *L) {
    aeEventLoop *event_loop;
    event_loop = (aeEventLoop *)lua_touserdata(L, 1); 
    aeMain(event_loop);
    return 0;
}

static int lwait(lua_State *L) {
    int fd;
    int mask;
    int result;
    long long milliseconds;
    fd = (int)lua_tointeger(L, 1);
    mask = (int)lua_tointeger(L, 2);
    milliseconds = (long long)lua_tonumber(L, 3);
    result = aeWait(fd, mask, milliseconds);
    lua_pushinteger(L, result);
    return 1;
}

static int lprocess_events(lua_State *L) {
    aeEventLoop *event_loop;
    int flags;
    int error;
    event_loop = (aeEventLoop *)lua_touserdata(L, 1); 
    flags = (int)lua_tointeger(L, 2);
    error = aeProcessEvents(event_loop, flags);
    lua_pushinteger(L, error);
    return 1;
}

static int ldelete_time_event(lua_State *L) {
    aeEventLoop *event_loop;
    long long id;
    int error;
    event_loop = (aeEventLoop *)lua_touserdata(L, 1); 
    id = (long long)lua_tonumber(L, 2);
    error = aeDeleteTimeEvent(event_loop, id);
    lua_pushinteger(L, error);
    return 1;
}

static int lcreate_time_event(lua_State *L) {
    aeEventLoop *event_loop;
    long long milliseconds;
    char *time_proc_name;
    char *finalizer_proc_name;
    event_loop = (aeEventLoop *)lua_touserdata(L, 1); 
    milliseconds = (long long)lua_tonumber(L, 2);
    time_proc_name = (char *)lua_tostring(L, 3);
    if (lua_isstring(L, 4)) {
        finalizer_proc_name = (char *)lua_tostring(L, 4);
    } else {
        finalizer_proc_name = NULL;
    }
    int idx = get_client_data_idx(L, NULL, time_proc_name, finalizer_proc_name);
    int id = aeCreateTimeEvent(event_loop, id, time_proc, (void *)idx, NULL);
    lua_pushnumber(L, id);
    return 1;
}

static int lget_file_events(lua_State *L) {
    aeEventLoop *event_loop;
    int sockfd;
    event_loop = (aeEventLoop *)lua_touserdata(L, 1); 
    sockfd = (int)lua_tointeger(L, 2);
    int mask = aeGetFileEvents(event_loop, sockfd);
    lua_pushinteger(L, mask);
    return 1;
}

static int ldelete_file_event(lua_State *L) {
    aeEventLoop *event_loop;
    int sockfd;
    int mask;
    event_loop = (aeEventLoop *)lua_touserdata(L, 1); 
    sockfd = (int)lua_tointeger(L, 2);
    mask = (int)lua_tointeger(L, 3);
    aeDeleteFileEvent(event_loop, sockfd, mask);
    return 0;
} 

static int lcreate_file_event(lua_State *L) {
    aeEventLoop *event_loop;
    int sockfd;
    int mask;
    int error;
    char *file_proc_name;
    event_loop = (aeEventLoop *)lua_touserdata(L, 1); 
    sockfd = (int)lua_tointeger(L, 2);
    mask = (int)lua_tointeger(L, 3);
    file_proc_name = (char *)lua_tostring(L, 4);
    int idx = get_client_data_idx(L, file_proc_name, NULL, NULL);
    error = aeCreateFileEvent(event_loop, sockfd, mask, file_proc, (int)idx);
    lua_pushinteger(L, error);
    return 1;
}

static int lcreate_event_loop(lua_State *L) {
    int setsize;
    setsize = (int)lua_tointeger(L, 1);
    aeEventLoop *event_loop = aeCreateEventLoop(setsize);
    lua_pushlightuserdata(L, event_loop);
    return 1;
}

static int ldelete_event_loop(lua_State *L) {
    aeEventLoop *event_loop;
    event_loop = (aeEventLoop *)lua_touserdata(L, 1); 
    aeDeleteEventLoop(event_loop);
    return 0;
}

static int lstop(lua_State *L) {
    aeEventLoop *event_loop;
    event_loop = (aeEventLoop *)lua_touserdata(L, 1); 
    aeStop(event_loop);
    return 0;
}

static int lset_before_sleep_proc(lua_State *L) {
    //aeEventLoop *event_loop;
    //event_loop = (aeEventLoop *)lua_touserdata(L, 1); 
    //aeSetBeforeSleepProc(event_loop, before_sleep_proc);  
    return 0;
}


static luaL_Reg lua_lib[] ={
    {"get_api_name", lget_api_name},
    {"main", lmain},
    {"run_once", lrun_once},
    {"stop", lstop},
    {"wait", lwait},
    {"get_file_events", lget_file_events},
    {"lprocess_events", lprocess_events},
    {"create_file_event", lcreate_file_event},
    {"delete_file_event", ldelete_file_event},
    {"create_time_event", lcreate_time_event},
    {"delete_time_event", ldelete_time_event},
    {"create_event_loop", lcreate_event_loop},
    {"delete_event_loop", ldelete_event_loop},
    {NULL, NULL}
};

int luaopen_ae(lua_State *L){
    luaL_newlib(L, lua_lib);

    lua_pushstring(L, "AE_NONE");
    lua_pushinteger(L, AE_NONE);
    lua_settable(L, -3);

    lua_pushstring(L, "AE_READABLE");
    lua_pushinteger(L, AE_READABLE);
    lua_settable(L, -3);

    lua_pushstring(L, "AE_WRITABLE");
    lua_pushinteger(L, AE_WRITABLE);
    lua_settable(L, -3);

	return 1;
}

