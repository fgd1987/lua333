
#include "queue.h"
#include "luas.h"

typedef struct Queue{
    int *elems;
    //申请的内存个数
    int capacity;
    //元素个数
    int count;
    int tail;
    int head;
}Queue;

static int Alloc(lua_State *L){
    if (lua_gettop(L) == 0){
        Queue *queue = (Queue *)lua_newuserdata(L, sizeof(Queue));
        if(queue == NULL){
            LOG_ERROR("newuserdata fail");
            lua_pushnil(L);
            return 1;
        }
        queue->elems = (int *)malloc(sizeof(int) * 1024);
        queue->capacity = 1024;
        queue->count = 0;
        queue->tail = 0;
        queue->head = 0;
        if(queue->elems == NULL){
            LOG_ERROR("malloc fail");
            lua_pushnil(L);
            return 1;
        }
        luaL_getmetatable(L, "QueueMetatable");
        lua_setmetatable(L, -2);
        return 1;
    }
    lua_pushnil(L);
    return 1;
}

static int Push(lua_State *L){
    if(lua_gettop(L) == 2 && lua_isuserdata(L, 1)){
        Queue *queue = (Queue *)luaL_checkudata(L, 1, "QueueMetatable");
        //扩充
        if(queue->count * 1.0 / queue->capacity > 0.5){
            int *new_elems = (int *)malloc(queue->capacity * 2 * sizeof(int));
            if(new_elems == NULL){
                lua_pushboolean(L, false);
                return 1;
            }
            memcpy(new_elems, queue->elems, queue->capacity * sizeof(int));
            queue->elems = new_elems;
            queue->capacity = queue->capacity * 2;
        }
        int ref = luaL_ref(L, LUA_REGISTRYINDEX);
        queue->elems[queue->tail] = ref;
        queue->tail++;
        queue->count++;
        if(queue->tail == queue->capacity){
            memmove(queue->elems, queue->elems + queue->head, queue->count * sizeof(int));
            queue->tail = queue->tail - queue->head;
            queue->head = 0;
        }
        lua_pushboolean(L, true);
        return 1;
    }
    lua_pushboolean(L, false);
    return 1;
}

static int Pop(lua_State *L){
    if(lua_gettop(L) == 1 && lua_isuserdata(L, 1)){
        Queue *queue = (Queue *)luaL_checkudata(L, 1, "QueueMetatable");
        if(queue->count == 0){
            lua_pushnil(L);
            return 1;
        }
        int ref = queue->elems[queue->head];
        lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
        queue->head++;
        queue->count--;
        if(queue->count == 0){
            queue->tail = queue->head = 0;
        }
        luaL_unref(L, LUA_REGISTRYINDEX, ref);
        return 1;
    }
    lua_pushnil(L);
    return 1;
}

static int Peek(lua_State *L){
    if(lua_gettop(L) == 1 && lua_isuserdata(L, 1)){
        Queue *queue = (Queue *)luaL_checkudata(L, 1, "QueueMetatable");
        if(queue->count == 0){
            lua_pushnil(L);
            return 1;
        }
        int ref = queue->elems[queue->head];
        lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
        return 1;
    }
    lua_pushnil(L);
    return 1;
}

static int Destory(lua_State *L){
    if(lua_gettop(L) == 1 && lua_isuserdata(L, 1)){
        Queue *queue = (Queue *)luaL_checkudata(L, 1, "QueueMetatable");
        for(int i = 0; i < queue->count; i++){
            int ref = queue->elems[queue->head + i];
            luaL_unref(L, LUA_REGISTRYINDEX, ref);
        }
        free(queue->elems);
    }
    return 0;
}

static luaL_Reg lua_lib[] = {
    {"Alloc", Alloc},
    {NULL, NULL}
};

static luaL_Reg lua_metatable[] = {
    {"Push", Push},
    {"Pop", Pop},
    {"Peek", Peek},
    {"__gc", Destory},
    {NULL, NULL}
};


int queue_init(lua_State *L){
    luaL_register(L, "Queue", (luaL_Reg*)lua_lib);
    luaL_newmetatable(L, "QueueMetatable");
    lua_pushstring(L, "__index");
    lua_pushvalue(L, -2);
    lua_settable(L, -3);
    luaL_register(L, NULL, (luaL_Reg*)lua_metatable);
    return 1;
}


