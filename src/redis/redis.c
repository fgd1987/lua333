#include "redis.h"
#include "adapters/ae.h"
#include "evt.h"


static void getCallback_push_reply(redisReply *reply){
    lua_newtable(L);

    lua_pushstring(L, "type");
    if(reply->type == REDIS_REPLY_NIL){
        lua_pushstring(L, "null");
    }else if(reply->type == REDIS_REPLY_INTEGER){
        lua_pushstring(L, "integer");
    }else if(reply->type == REDIS_REPLY_STRING){
        lua_pushstring(L, "string");
    }else if(reply->type == REDIS_REPLY_ERROR){
        lua_pushstring(L, "error");
    }else if(reply->type == REDIS_REPLY_STATUS){
        lua_pushstring(L, "status");
    }else if(reply->type == REDIS_REPLY_ARRAY){
        lua_pushstring(L, "array");
    }

    lua_settable(L, -3);

    lua_pushstring(L, "value");
    if (reply->type == REDIS_REPLY_NIL) {
        lua_pushnil(L);
    }else if (reply->type == REDIS_REPLY_INTEGER) {
        lua_pushnumber(L, reply->integer);
    }else if (reply->type == REDIS_REPLY_STRING) {
        lua_pushlstring(L, reply->str, reply->len);
    }else if (reply->type == REDIS_REPLY_ERROR) {
        lua_pushstring(L, reply->str);
    }else if (reply->type == REDIS_REPLY_STATUS) {
        lua_pushstring(L, reply->str);
    }else if (reply->type == REDIS_REPLY_ARRAY) {
        lua_newtable(L);
        redisReply **elements = reply->element;
        for(unsigned int i = 0; i < reply->elements; i++){
           redisReply *reply = elements[i];
           lua_pushnumber(L, i + 1);
           getCallback_push_reply(reply);
           lua_settable(L, -3);
        }  
    }
    lua_settable(L, -3);
} 
static void getCallback(redisAsyncContext *c, void *r, void *privdata) {
    redisReply *reply = (redisReply *)r;
    if (reply == NULL) {
        LOG_ERROR("getCallback reply null");
        return;
    }

    luas_pushluafunction((char *)privdata);

    getCallback_push_reply(reply);

    if (lua_pcall(L, 1, 0, 0) != 0){
        LOG_ERROR("error running function %s: %s", privdata, lua_tostring(L, -1));
    }
}

static void connectCallback(const redisAsyncContext *c, int status) {
    if (status != REDIS_OK && strlen(c->errstr) > 0) { //TODO 要判断errstr??
        LOG_ERROR("disconnect error:%s", c->errstr);
        return;
    }
    AsyncRedis *redis = (AsyncRedis *)c->data;
    luas_pushluafunction((char *)redis->cb_connect);
    if (lua_pcall(L, 0, 0, 0) != 0){
        LOG_ERROR("error running function %s: %s", redis->cb_connect, lua_tostring(L, -1));
    }
}

static void disconnectCallback(const redisAsyncContext *c, int status) {
    AsyncRedis *redis = (AsyncRedis *)c->data;
    luas_pushluafunction((char *)redis->cb_disconnect);
    lua_pushstring(L, c->errstr);
    if (lua_pcall(L, 1, 0, 0) != 0){
        LOG_ERROR("error running function %s: %s", redis->cb_disconnect, lua_tostring(L, -1));
    }

    redis->c = redisAsyncConnect(redis->ip, redis->port);
    if(redis->c == NULL){
        LOG_ERROR("redisAsyncConnect fail");
        return;
    }
    redis->c->data = redis;//PS, userdata
    if (redis->c->err) {
        LOG_ERROR("redisAsyncConnect fail : %s", redis->c->errstr);
        return;
    }
    redisAeAttach(evt_loop(), redis->c);
    redisAsyncSetDisconnectCallback(redis->c,disconnectCallback);
    redisAsyncSetConnectCallback(redis->c,connectCallback);
}

static int ReConnect(lua_State *L){
    if(lua_gettop(L) == 1 && lua_isuserdata(L, 1)){
        Redis *redis = (Redis *)luaL_checkudata(L, 1, "RedisMetatable");
        if(redis == NULL){
            LOG_ERROR("newuserdata fail");
            return 0;
        }
        if(redis->c != NULL){
            redisFree(redis->c);
            redis->c = NULL;
        }
        redis->c = redisConnect(redis->ip, redis->port);
        if(redis->c == NULL){
            LOG_ERROR("connect error\n");
            return 0;
        }
        if(!(redis->c->flags & REDIS_CONNECTED)){
            redisFree(redis->c);
            redis->c = NULL;
            LOG_ERROR("connect error");
            return 0;
        }
        lua_pushboolean(L, true);
        return 1;
    }
    lua_pushboolean(L, false);
    return 1;
}
static int Connect(lua_State *L){
    if(lua_gettop(L) == 2 && lua_isstring(L, 1) && lua_isnumber(L, 2)){
        const char *ip = (const char *)lua_tostring(L, 1);
        unsigned short port = (unsigned short)lua_tonumber(L, 2);
        redisContext *c = redisConnect(ip, port);
        if(c == NULL){
            LOG_ERROR("connect error");
            return 0;
        }
        if(!(c->flags & REDIS_CONNECTED)){
            redisFree(c);
            LOG_ERROR("connect error");
            return 0;
        }
        Redis *redis = (Redis *)lua_newuserdata(L, sizeof(Redis));
        if(redis == NULL){
            redisFree(c);
            LOG_ERROR("newuserdata FAIL");
            return 0;
        }
        redis->c = c;
        strcpy(redis->ip, ip);
        redis->port = port;
        luaL_getmetatable(L, "RedisMetatable");
        lua_setmetatable(L, -2);
        return 1;
    }
    lua_pushnil(L);
    return 1;
}
static int Async(lua_State *L){
    if(lua_gettop(L) == 0){
        AsyncRedis *redis = (AsyncRedis *)lua_newuserdata(L, sizeof(AsyncRedis));
        if(redis == NULL){
            LOG_ERROR("userdata is null");
            return 0;
        }
        redis->c = NULL;
        luaL_getmetatable(L, "AsyncRedisMetatable");
        lua_setmetatable(L, -2);
        return 1;
    }
    lua_pushnil(L);
    return 1;
}
static int CloseAsyncRedis(lua_State *L){
    if(lua_gettop(L) == 1 && lua_isuserdata(L, 1)){
        AsyncRedis *redis = (AsyncRedis *)luaL_checkudata(L, 1, "AsyncRedisMetatable");
        if(redis->c){
            redisAsyncDisconnect(redis->c);
            redis->c = NULL;
        }
        return 0;
    }
    return 0;
}

static int AsyncConnect(lua_State *L){
    if(!lua_isuserdata(L, 1) || !lua_isstring(L, 2) || !lua_isnumber(L, 3)){
        LOG_ERROR("arg error");
        return 0;
    }
    AsyncRedis *redis = (AsyncRedis *)luaL_checkudata(L, 1, "AsyncRedisMetatable");
    if(redis == NULL){
        LOG_ERROR("userdata is null");
        return 0;
    }
    if(redis->c){
        LOG_ERROR("is connected");
        return 0;
    }
    const char *ip = (const char *)lua_tostring(L, 2);
    unsigned short port = (unsigned short)lua_tonumber(L, 3);
    strcpy(redis->ip, ip);
    redis->port = port;
    redis->c = redisAsyncConnect(ip, port);
    if(redis->c == NULL){
        LOG_ERROR("redisAsyncConnect fail");
        return 0;
    }
    redis->c->data = redis;//PS, userdata
    if (redis->c->err) {
        LOG_ERROR("redisAsyncConnect fail : %s", redis->c->errstr);
        return 0;
    }
    redisAeAttach(evt_loop(), redis->c);
    redisAsyncSetDisconnectCallback(redis->c,disconnectCallback);
    redisAsyncSetConnectCallback(redis->c,connectCallback);
    lua_pushboolean(L, true);
    return 1;
}
static int AsyncHSet(lua_State *L){
    if (lua_gettop(L) == 4 && lua_isuserdata(L, 1) && lua_isstring(L, 2) && lua_isstring(L, 3) && lua_isstring(L, 4)){
        AsyncRedis *redis = (AsyncRedis *)luaL_checkudata(L, 1, "AsyncRedisMetatable");
        if (redis == NULL) {
            LOG_ERROR("userdata is null");
            return 0;
        }
        const char *table = (const char *)LUA_TOSTRING(2);
        const char *key = (const char *)LUA_TOSTRING(3);
        const char *str = (const char *)LUA_TOSTRING(4);
        redisAsyncCommand((redisAsyncContext *)redis->c, NULL, NULL, "hset %s %s %s", table, key, str);
       
        lua_pushboolean(L, true);
        return 1;
    }
    lua_pushboolean(L, false);
    return 1;
}

/*异步SET*/
static int AsyncSet(lua_State *L){
    if (lua_gettop(L) == 3 && lua_isuserdata(L, 1) && lua_isstring(L, 2) && lua_isstring(L, 3)) {
        AsyncRedis *redis = (AsyncRedis *)luaL_checkudata(L, 1, "AsyncRedisMetatable");
        if(redis == NULL){
            LOG_ERROR("userdata is null");
            return 0;
        }
        const char *key = (const char *)LUA_TOSTRING(2);
        const char *str = (const char *)LUA_TOSTRING(3);
        redisAsyncCommand((redisAsyncContext *)redis->c, NULL, NULL, "set %s %s", key, str);

        lua_pushboolean(L, true);
        return 1;
    }
    lua_pushboolean(L, false);
    return 1;
}
/*异步COMMAND, 可以设置一个回调函数*/
static int AsyncCommand(lua_State *L){
    if (lua_isuserdata(L, 1) && lua_isstring(L, 2)) {
        AsyncRedis *redis = (AsyncRedis *)luaL_checkudata(L, 1, "AsyncRedisMetatable");
        if (redis == NULL) {
            LOG_ERROR("userdata is null");
            return 0;
        }
        const char *command = (const char *)lua_tostring(L, 2);
        if (lua_isstring(L, 3)) {
            const char *callback =  (const char *)lua_tostring(L, 3);
            redisAsyncCommand((redisAsyncContext *)redis->c, getCallback, (char*)callback, command);
        } else {
            redisAsyncCommand((redisAsyncContext *)redis->c, NULL, NULL, command);
        }
        lua_pushboolean(L, true);
        return 1;
    }
    lua_pushboolean(L, false);
    return 1;
}

static int CloseRedis(lua_State *L){
    if (lua_gettop(L) == 1 && lua_isuserdata(L, 1)) {
        Redis *redis = (Redis *)luaL_checkudata(L, 1, "RedisMetatable");
        redisFree(redis->c);
        return 0;
    }
    lua_pushboolean(L, false);
    return 1;
}
static int Zrevrank(lua_State *L){
   
    if (!lua_isuserdata(L, 1) || !lua_isstring(L, 2) || !lua_isstring(L, 3)) {
        LOG_ERROR("arg error");
        return 0;
    }
    Redis *redis = (Redis *)luaL_checkudata(L, 1, "RedisMetatable");
    if(redis == NULL){
        LOG_ERROR("userdata is null");
        return 0;
    }
    const char *key = (const char *)lua_tostring(L, 2);


    size_t str_len;
    const char *value = (const char *)lua_tolstring(L, 3, &str_len);
    if(value == NULL){
        LOG_ERROR("value fail");
        return 0;
    }
    redisReply * reply = (redisReply *)redisCommand((redisContext *)redis->c, "ZREVRANK %s %b", key, value, str_len);
    if(reply == NULL){
        LOG_ERROR("Set:redisCommand fail key:%s value:%s", key, value);
        return 0;
    }
    getCallback_push_reply(reply);
    freeReplyObject(reply);
    return 1;
}


static int Zrem(lua_State *L){
   
    if (!lua_isuserdata(L, 1) || !lua_isstring(L, 2) || !lua_isstring(L, 3)) {
        LOG_ERROR("arg error");
        return 0;
    }
    Redis *redis = (Redis *)luaL_checkudata(L, 1, "RedisMetatable");
    if(redis == NULL){
        LOG_ERROR("userdata is null");
        return 0;
    }
    const char *key = (const char *)lua_tostring(L, 2);


    size_t str_len;
    const char *value = (const char *)lua_tolstring(L, 3, &str_len);
    if(value == NULL){
        LOG_ERROR("value fail");
        return 0;
    }
    redisReply * reply = (redisReply *)redisCommand((redisContext *)redis->c, "ZREM %s %b", key, value, str_len);
    if(reply == NULL){
        LOG_ERROR("Set:redisCommand fail key:%s value:%s", key, value);
        return 0;
    }
    getCallback_push_reply(reply);
    freeReplyObject(reply);
    return 1;
}


static int Zrank(lua_State *L){
   
    if (!lua_isuserdata(L, 1) || !lua_isstring(L, 2) || !lua_isstring(L, 3)) {
        LOG_ERROR("arg error");
        return 0;
    }
    Redis *redis = (Redis *)luaL_checkudata(L, 1, "RedisMetatable");
    if(redis == NULL){
        LOG_ERROR("userdata is null");
        return 0;
    }
    const char *key = (const char *)lua_tostring(L, 2);


    size_t str_len;
    const char *value = (const char *)lua_tolstring(L, 3, &str_len);
    if(value == NULL){
        LOG_ERROR("value fail");
        return 0;
    }
    redisReply * reply = (redisReply *)redisCommand((redisContext *)redis->c, "ZRANK %s %b", key, value, str_len);
    if(reply == NULL){
        LOG_ERROR("Set:redisCommand fail key:%s value:%s", key, value);
        return 0;
    }
    getCallback_push_reply(reply);
    freeReplyObject(reply);
    return 1;
}


static int Zadd(lua_State *L){
   
    if (!lua_isuserdata(L, 1) || !lua_isstring(L, 2) || !lua_isnumber(L ,3) || !lua_isstring(L, 4)) {
        LOG_ERROR("arg error");
        return 0;
    }
    Redis *redis = (Redis *)luaL_checkudata(L, 1, "RedisMetatable");
    if(redis == NULL){
        LOG_ERROR("userdata is null");
        return 0;
    }
    const char *key = (const char *)lua_tostring(L, 2);

    float score = (float)lua_tonumber(L, 3);

    size_t str_len;
    const char *value = (const char *)lua_tolstring(L, 4, &str_len);
    if(value == NULL){
        LOG_ERROR("value fail");
        return 0;
    }
    redisReply * reply = (redisReply *)redisCommand((redisContext *)redis->c, "ZADD %s %f %b", key, score, value, str_len);
    if(reply == NULL){
        LOG_ERROR("Set:redisCommand fail key:%s value:%s", key, value);
        return 0;
    }
    getCallback_push_reply(reply);
    freeReplyObject(reply);
    return 1;
}

static int Set(lua_State *L){
   
    if (!lua_isuserdata(L, 1) || !lua_isstring(L, 2) || !lua_isstring(L ,3)) {
        LOG_ERROR("arg error");
        return 0;
    }
    Redis *redis = (Redis *)luaL_checkudata(L, 1, "RedisMetatable");
    if(redis == NULL){
        LOG_ERROR("userdata is null");
        return 0;
    }
    const char *key = (const char *)lua_tostring(L, 2);
    size_t str_len;
    const char *value = (const char *)lua_tolstring(L, 3, &str_len);
    if(value == NULL){
        LOG_ERROR("value fail");
        return 0;
    }
    redisReply * reply = (redisReply *)redisCommand((redisContext *)redis->c, "set %s %b", key, value, str_len);
    if(reply == NULL){
        LOG_ERROR("Set:redisCommand fail key:%s value:%s", key, value);
        return 0;
    }
    getCallback_push_reply(reply);
    freeReplyObject(reply);
    return 1;
}
static int LPush(lua_State *L){
    if (!lua_isuserdata(L, 1) || !lua_isstring(L, 2) || !lua_isstring(L ,3)) {
        LOG_ERROR("arg error");
        return 0;
    }
    Redis *redis = (Redis *)luaL_checkudata(L, 1, "RedisMetatable");
    if (redis == NULL) {
        LOG_ERROR("userdata is null");
        return 0;
    }
    const char *key = (const char *)lua_tostring(L, 2);
    size_t str_len;
    const char *str = (const char *)lua_tolstring(L, 3, &str_len);
    redisReply * reply = (redisReply *)redisCommand((redisContext *)redis->c, "LPUSH %s %b", key, str, str_len);
    if (reply == NULL) {
        LOG_ERROR("lpush fail key:%s value:%s", key, str);
        return 0;
    }
    getCallback_push_reply(reply);
    freeReplyObject(reply);
    return 1;
}


static int HSet(lua_State *L)
{
   
    if (!lua_isuserdata(L, 1) || !lua_isstring(L, 2) || !lua_isstring(L ,3) || !lua_isstring(L ,4)) {
        LOG_ERROR("arg error");
        return 0;
    }
    Redis *redis = (Redis *)luaL_checkudata(L, 1, "RedisMetatable");
    if (redis == NULL) {
        LOG_ERROR("userdata is null");
        return 0;
    }
    const char *table = (const char *)lua_tostring(L, 2);
    const char *key = (const char *)lua_tostring(L, 3);
    const char *value = (const char *)lua_tostring(L, 4);
    redisReply * reply = (redisReply *)redisCommand((redisContext *)redis->c, "hset %s %s %s", table, key, value);
    if (reply == NULL) {
        LOG_ERROR("hset fail table:%s key:%s value:%s", table, key, value);
        return 0;
    }
    getCallback_push_reply(reply);
    freeReplyObject(reply);
    return 1;
}

static char **redis_argv;
static size_t *redis_argvlen;
static int redis_argc;
static char s_hmset_cmd[] = "HMSET";
static char s_mset_cmd[] = "MSET";

static int MSet(lua_State *L){
    if (!lua_isuserdata(L, 1)) {
        LOG_ERROR("arg error");
        return 0;
    }
    Redis *redis = (Redis *)luaL_checkudata(L, 1, "RedisMetatable");
    if(redis == NULL){
        LOG_ERROR("userdata is null");
        return 0;
    }
    int argc = lua_gettop(L);
    if(redis_argc < argc){
        if(redis_argv != NULL){
            redis_argc = 0;
            free(redis_argv);
        }
        if(redis_argvlen != NULL){
            redis_argc = 0;
            free(redis_argvlen);
        }
        redis_argv = (char **)malloc(argc * sizeof(char *));
        redis_argvlen = (size_t *)malloc(argc * sizeof(size_t));
        if(!redis_argv || !redis_argvlen){
            redis_argv = 0;
            if(redis_argv)free(redis_argv);
            if(redis_argvlen)free(redis_argvlen);
            LOG_ERROR("malloc fail");
            return 0;
        }
        redis_argc = argc;
    }
    redis_argv[0] = s_mset_cmd;
    redis_argvlen[0] = 4;
    for(int i = 0; i < (argc - 1)/2; i++){
        size_t k_len = 0;
        size_t v_len = 0;
        const char *k = (const char *)lua_tolstring(L, (i*2) + 2, &k_len);
        const char *v = (const char *)lua_tolstring(L, (i*2) + 3, &v_len);
        redis_argv[(2*i)+1] = (char *)k;
        redis_argvlen[(2*i)+1] = k_len;
        redis_argv[(2*i)+2] = (char *)v;
        redis_argvlen[(2*i)+2] = v_len;
    }
    redisReply * reply = (redisReply *)redisCommandArgv((redisContext *)redis->c, argc, (const char **)redis_argv, redis_argvlen);
    if(reply == NULL){
        LOG_ERROR("MSET FAIL");
        return 0;
    }

    getCallback_push_reply(reply);
    freeReplyObject(reply);
    return 1;
}

static int HMSet(lua_State *L){
    if (!lua_isuserdata(L, 1)) {
        LOG_ERROR("arg error");
        return 0;
    }
    Redis *redis = (Redis *)luaL_checkudata(L, 1, "RedisMetatable");
    if(redis == NULL){
        LOG_ERROR("userdata is null");
        return 0;
    }
    int argc = lua_gettop(L);
    if(argc % 2 != 0){
        LOG_ERROR("expect even but %d", argc);
        return 0;
    }
    if(redis_argc < argc){
        if(redis_argv != NULL){
            redis_argc = 0;
            free(redis_argv);
        }
        if(redis_argvlen != NULL){
            redis_argc = 0;
            free(redis_argvlen);
        }
        redis_argv = (char **)malloc(argc * sizeof(char *));
        redis_argvlen = (size_t *)malloc(argc * sizeof(size_t));
        if(!redis_argv || !redis_argvlen){
            redis_argv = 0;
            if(redis_argv)free(redis_argv);
            if(redis_argvlen)free(redis_argvlen);
            LOG_ERROR("malloc fail");
            return 0;
        }
        redis_argc = argc;
    }
    size_t key_len;
    const char *key = (const char *)lua_tolstring(L, 2, &key_len);
    if(key == NULL){
        LOG_ERROR("tolstring FAIL");
        return 0;
    }
    redis_argv[0] = s_hmset_cmd;
    redis_argvlen[0] = 5;
    redis_argv[1] = (char *)key;
    redis_argvlen[1] = key_len;

    for(int i = 0; i < (argc - 1)/2; i++){
        size_t k_len = 0;
        size_t v_len = 0;
        const char *k = (const char *)lua_tolstring(L, (i * 2) + 3, &k_len);
        const char *v = (const char *)lua_tolstring(L, (i * 2) + 4, &v_len);
        redis_argv[2*i + 2] = (char *)k;
        redis_argvlen[2*i + 2] = k_len;
        redis_argv[2*i + 3] = (char *)v;
        redis_argvlen[2*i + 3] = v_len;
    }
    redisReply * reply = (redisReply *)redisCommandArgv((redisContext *)redis->c, argc, (const char **)redis_argv, redis_argvlen);
    if(reply == NULL){
        LOG_ERROR("MSET FAIL");
        return 0;
    }

    getCallback_push_reply(reply);
    freeReplyObject(reply);
    return 1;
}



static int Command(lua_State *L)
{
   
    if (!lua_isuserdata(L, 1) || !lua_isstring(L, 2)) {
        LOG_ERROR("arg error");
        return 0;
    }
    Redis *redis = (Redis *)luaL_checkudata(L, 1, "RedisMetatable");
    if(redis == NULL){
        LOG_ERROR("userdata is null");
        return 0;
    }
    size_t str_len = 0;
    const char *command = (const char *)lua_tolstring(L, 2, &str_len);
    redisReply * reply = (redisReply *)redisCommand((redisContext *)redis->c, command);
    if(reply == NULL){
        LOG_ERROR("Command:redisCommand fail command:%s", command);
        return 0;
    }

    getCallback_push_reply(reply);
    freeReplyObject(reply);
    return 1;
}


static int OnConnect(lua_State *L){
    if (!lua_isuserdata(L, 1) || !lua_isstring(L, 2)) {
        LOG_ERROR("arg error");
        return 0;
    }
    AsyncRedis *redis = (AsyncRedis *)luaL_checkudata(L, 1, "AsyncRedisMetatable");
    if(redis == NULL){
        LOG_ERROR("userdata is null");
        return 0;
    }
    const char *script = (const char *)lua_tostring(L, 2);
    strcpy(redis->cb_connect, script);
    lua_pushboolean(L, true);
    return 1;
}

static int OnDisconnect(lua_State *L){
    if (!lua_isuserdata(L, 1) || !lua_isstring(L, 2)) {
        LOG_ERROR("arg error");
        return 0;
    }
    AsyncRedis *redis = (AsyncRedis *)luaL_checkudata(L, 1, "AsyncRedisMetatable");
    if (redis == NULL) {
        LOG_ERROR("userdata is null");
        return 0;
    }
    const char *script = (const char *)lua_tostring(L, 2);
    strcpy(redis->cb_disconnect, script);
    lua_pushboolean(L, true);
    return 1;
}
static luaL_Reg lua_lib[] = {
    {"Async", Async},
    {"Connect", Connect},
    {NULL, NULL}
};

static luaL_Reg lua_metatable[] = {
    {"Command", Command},
    {"ReConnect", ReConnect},
    {"MSet", MSet},
    {"HMSet", HMSet},
    {"Set", Set},
    {"Zadd", Zadd},
    {"Zrank", Zrank},
    {"Zrevrank", Zrevrank},
    {"Zrem", Zrem},
    {"HSet", HSet},
    {"LPush", LPush},
    {"__gc", CloseRedis},
    {NULL, NULL}
};


static luaL_Reg lua_async_metatable[] = {
    {"Connect", AsyncConnect},
    {"Command", AsyncCommand},

    {"Set", AsyncSet},
    {"HSet", AsyncHSet},

    {"OnConnect", OnConnect},
    {"OnDisconnect", OnDisconnect},
    {"__gc", CloseAsyncRedis},
    {NULL, NULL}
};


int redis_init(lua_State *L){
	luaL_register(L, "Redis", (luaL_Reg*)lua_lib);
   
    //阻塞redis
    luaL_newmetatable(L, "RedisMetatable");
    lua_pushstring(L, "__index");
    lua_pushvalue(L, -2);
    lua_settable(L, -3);
    luaL_register(L, NULL, (luaL_Reg*)lua_metatable);

    //非阻塞redis
    luaL_newmetatable(L, "AsyncRedisMetatable");
    lua_pushstring(L, "__index");
    lua_pushvalue(L, -2);
    lua_settable(L, -3);
    luaL_register(L, NULL, (luaL_Reg*)lua_async_metatable);
   
    return 1;
}

