extern "C"{
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}
#include "log.h"
#include "rmi_port.h"
#include "bbuf.h"
#include "ar.h"
#include "pbc.h"
#include "port.h"
#include "json.h"
#include <stdio.h>
#include <string.h>

/*
plen mod method arg_num
{unsigned char, str}
1, string
2, double
3, table -json
4, pb

*/
/*
 *  POST(sockd).game_srv.login(game_srv_id, game_srv_name)
 *  RMIPort.Broadcast(portfd).game_srv.login(game_srv_id, game_srv_name)
 *
 */

#define ARG_TYPE_END    0
#define ARG_TYPE_STRING 1 
#define ARG_TYPE_NUMBER 2
#define ARG_TYPE_TABLE 3
#define ARG_TYPE_PB 4
#define ARG_TYPE_NIL 5

#define MAX_MOD_NAME 64
#define MAX_ACTION_NAME  128


#define READ_UINT32(buf, buf_len) *(unsigned int *)buf;\
                                 if(buf_len < sizeof(unsigned int)){\
                                    LOG_ERROR("parse fail");\
                                    return 0;\
                                 }\
                                 buf += sizeof(unsigned int);\
                                 buf_len -= sizeof(unsigned int);

#define READ_UINT16(buf, buf_len) *(unsigned short *)buf;\
                                 if(buf_len < sizeof(unsigned short)){\
                                    LOG_ERROR("parse fail");\
                                    return 0;\
                                 }\
                                 buf += sizeof(unsigned short);\
                                 buf_len -= sizeof(unsigned short);

#define READ_UINT8(buf, buf_len) *(unsigned char *)buf;\
                                 if(buf_len < sizeof(unsigned char)){\
                                    LOG_ERROR("parse fail");\
                                    return 0;\
                                 }\
                                 buf += sizeof(unsigned char);\
                                 buf_len -= sizeof(unsigned char);


#define READ_STR(buf, buf_len, src, str_len)  if(buf_len < str_len){\
                                                 LOG_ERROR("parse fail");\
                                                 return 0;\
                                              }\
                                              memcpy(src, buf, str_len);\
                                              src[str_len] = 0;\
                                              buf += str_len;\
                                              buf_len -= str_len;


static int time_diff(struct timeval *t1, struct timeval *t2){
    int usec = (t2->tv_sec - t1->tv_sec) * 1000000 + t2->tv_usec - t1->tv_usec;
    return usec;
}


#define MAX_MSG_NAME 1024
static char msg_name[MAX_MSG_NAME];
typedef struct RMIMessage{
    union{
        int linefd;
        int portfd;
    };
    int timeout_sec; //GET
    char mod_name[MAX_MOD_NAME];
    char action_name[MAX_ACTION_NAME];
}RMIMessage;

RMIMessage g_rmi_message;

static void setnonblock(int sockfd){
    int flags;
    flags = fcntl(sockfd, F_GETFL);
    flags |= O_NONBLOCK;
    fcntl(sockfd, F_SETFL, flags);
}
static void setblock(int sockfd){
    int flags;
    flags = fcntl(sockfd, F_GETFL);
    flags &= ~O_NONBLOCK;
    fcntl(sockfd, F_SETFL, flags);
}

static int rmi_port_cb_packet(int portfd, int linefd, int body){


    struct timeval t1;
    gettimeofday(&t1, NULL);

    Port *port = port_from_fd(portfd);
    char *buf = ar_ptr(body);
    unsigned int buf_len = ar_remain(body);
    
    //LOG_DEBUG("buf_len:%d\n", buf_len);
    lua_State *L = luas_state();

    unsigned int body_len = READ_UINT32(buf, buf_len);
    RMIMessage rmi_message;
    unsigned short mod_name_len = READ_UINT16(buf, buf_len);
    if(mod_name_len >= MAX_MOD_NAME){
        LOG_ERROR("limit reach");
        return 0;
    }
    READ_STR(buf, buf_len, rmi_message.mod_name, mod_name_len);
    

    unsigned short action_name_len = READ_UINT16(buf, buf_len);
    if(action_name_len >= MAX_ACTION_NAME){
        LOG_ERROR("limit reach %d", action_name_len);
        return 0;
    }
    READ_STR(buf, buf_len, rmi_message.action_name, action_name_len);


    LOG_MSG("[POST_RECV]%s %s.%s", port->name, rmi_message.mod_name, rmi_message.action_name);

    lua_getglobal(L, rmi_message.mod_name);
    if(lua_isnil(L, -1)){
        LOG_ERROR("mod %s not found", rmi_message.mod_name);
        return 0;
    }

    lua_pushlstring(L, rmi_message.action_name, action_name_len);
    lua_gettable(L, -2);
    if(lua_isnil(L, -1)){
        LOG_ERROR("%s not found", rmi_message.action_name);
        return 0;
    }
    lua_pushinteger(L, linefd);
    int arg_num = 1;
    int max_arg_num = 20;
    while(true){
        if(max_arg_num -- <= 0){
            LOG_ERROR("arg num reach limit");
            break;
        }
        unsigned char t = READ_UINT8(buf, buf_len);
        if(t == ARG_TYPE_STRING){
            arg_num++;
            unsigned short str_len = READ_UINT16(buf, buf_len);
            lua_pushlstring(L, buf, str_len);

            char tmp_c = buf[str_len];
            buf[str_len] = 0;
            LOG_DEBUG("%s", buf);
            buf[str_len] = tmp_c;

            buf += str_len;
            buf_len -= str_len;
        }else if(t == ARG_TYPE_NUMBER){
            arg_num++;
            int number = *(int *)buf;
            buf += sizeof(int);
            buf_len -= sizeof(int);
            lua_pushnumber(L, number);
            LOG_DEBUG("%ld", (size_t)number);
        }else if(t == ARG_TYPE_TABLE){
            arg_num++;
            unsigned int str_len = READ_UINT32(buf, buf_len);
            json_decode(L, buf);

            char tmp_c = buf[str_len];
            buf[str_len] = 0;
            LOG_DEBUG("%s", buf);
            buf[str_len] = tmp_c;

            buf += str_len;
            buf_len -= str_len;
            //LOG_DEBUG("lua table %d, %d %d", str_len, buf_len, sizeof(int));
        }else if(t == ARG_TYPE_NIL){
            arg_num++;
            lua_pushnil(L);
            LOG_DEBUG("nil");
        }else if(t == ARG_TYPE_PB){
            arg_num++;
            unsigned int str_len = READ_UINT32(buf, buf_len);
            unsigned short msg_name_len = READ_UINT16(buf, buf_len);
            if(msg_name_len >= MAX_MSG_NAME){
                LOG_ERROR("msg name length limit reach");
                return 0;
            }
            READ_STR(buf, buf_len, msg_name, msg_name_len);
            google::protobuf::Message* message = pbc_load_msg(msg_name);
            if(message == NULL){
                LOG_ERROR("can not load msg %s", msg_name);
                return 0;
            }
            int msg_len = str_len - msg_name_len - sizeof(unsigned short);
            //printf("%s %d\n", msg_name, msg_len);
            google::protobuf::io::ArrayInputStream stream(buf, msg_len);
            if(message->ParseFromZeroCopyStream(&stream) == 0){
                LOG_ERROR("parse fail\n");
                return 0;
            }    
            buf += msg_len;
            LuaMessage *message_lua = (LuaMessage *)lua_newuserdata(L, sizeof(LuaMessage));
            if(message_lua == NULL){
                LOG_ERROR("newuserdata null %s", msg_name);
                return 0;
            }
            message_lua->message = message;
            message_lua->root_message = message_lua;
            message_lua->dirty = 0;
            luaL_getmetatable(L, "LuaMessage");
            lua_setmetatable(L, -2);
            if(message->ByteSize() < 1024){
                LOG_DEBUG("%s", message->DebugString().data());
            }
        }else if(t == ARG_TYPE_END){
            break;
        }else{
            LOG_ERROR("unsport : %d", t);
            return 0;
        }
    }
    if(lua_pcall(L, arg_num, 0, 0)){
        LOG_ERROR("%s", lua_tostring(L, -1));
    }
    struct timeval t2;
    gettimeofday(&t2, NULL);
    LOG_MSG("[POST_RECV_END] usec:%d body_len:%d", time_diff(&t1, &t2), body_len);
    lua_pop(L, lua_gettop(L));
    return 1;
}
static int rmi_strlen(lua_State *L, RMIMessage *rmi_message){

    int body_len = 0;
    int mod_name_len = strlen(rmi_message->mod_name);
    int action_name_len = strlen(rmi_message->action_name);
    body_len += 2 + mod_name_len;
    body_len += 2 + action_name_len;
    int top = lua_gettop(L);
    //LOG_DEBUG("body_len:%d", body_len);
    for(int i = 2; i <= top; i++){
        if(lua_type(L, i) == LUA_TNUMBER){
            body_len += sizeof(unsigned char);
            body_len += sizeof(int); 
            //LOG_DEBUG("body_len:%d %d", body_len, sizeof(int));
        }
        else if(lua_type(L, i) == LUA_TSTRING){
            size_t str_len = 0;
            lua_tolstring(L, i, &str_len);
            body_len += sizeof(unsigned char);
            body_len += sizeof(unsigned short);
            body_len += str_len;
            //LOG_DEBUG("body_len:%d", body_len);
        }else if(lua_isuserdata(L, i)){
            LuaMessage *message_lua = (LuaMessage *)luaL_checkudata(L, i, "LuaMessage");
            if(message_lua == NULL){
                LOG_ERROR("checkuserdata is null");
                return 0;
            }
            google::protobuf::Message *message = message_lua->message;
            if(message == NULL){
                LOG_ERROR("message is null");
                return 0;
            }
            const char *msg_name = message->GetDescriptor()->full_name().data();
            if(msg_name == NULL){
                LOG_ERROR("cmd is null");
                return 0;
            }
            body_len += sizeof(unsigned char); //type
            body_len += sizeof(unsigned int);  //len
            body_len += sizeof(unsigned short);//msg name len
            body_len += strlen(msg_name);      //msg name
            body_len += message->ByteSize();   //msg
        }else if(lua_istable(L, i)){
            lua_pushvalue(L, i);
            char *str = json_encode(L);
            lua_pop(L, 1);
            if(str == NULL){
                LOG_ERROR("fail");
                return 0;
            }
            int str_len = strlen(str);
            body_len += sizeof(unsigned char);
            body_len += sizeof(unsigned int);
            body_len += str_len;
            //printf("table len:%d\n", str_len);
            //释放
            free(str);
            //LOG_DEBUG("body_len:%d", body_len);
        }else if(lua_isnil(L, i)){
            body_len += sizeof(unsigned char);
        }
        else{
            LOG_ERROR("unsupport");
            return 0;
        }
    }
    body_len += sizeof(unsigned char);//end
    //LOG_DEBUG("body_len:%d", body_len);
    return body_len;
}


static int rmi_encode(lua_State *L, RMIMessage *rmi_message, char *buf, int buf_len){

    int mod_name_len = strlen(rmi_message->mod_name);
    int action_name_len = strlen(rmi_message->action_name);
    
    *(unsigned int *)buf = buf_len - sizeof(unsigned int);
    buf += sizeof(unsigned int);

    *(unsigned short *)buf = mod_name_len;
    buf += sizeof(unsigned short);
    memcpy(buf, rmi_message->mod_name, mod_name_len);
    buf += mod_name_len;

    *(unsigned short *)buf = action_name_len;
    buf += sizeof(unsigned short);
    memcpy(buf, rmi_message->action_name, action_name_len);
    buf += action_name_len;
    
    int top = lua_gettop(L);
    for(int i = 2; i <= top; i++){
        if(lua_type(L, i) == LUA_TNUMBER){
            *(unsigned char *)buf = ARG_TYPE_NUMBER;
            buf += sizeof(unsigned char);
            int number = lua_tonumber(L, i);
            *(int*)buf = number;
            buf += sizeof(int); 

            LOG_DEBUG("%ld", (size_t)number);
        }else if(lua_type(L, i) == LUA_TSTRING){
            *(unsigned char *)buf = ARG_TYPE_STRING;
            buf += sizeof(unsigned char);
            size_t str_len = 0;
            const char * str = (const char *)lua_tolstring(L, i, &str_len);
            if(str == NULL){
                LOG_ERROR("fail");
                return 0;
            }else{
                LOG_DEBUG("%s", str);
                *(unsigned short *)buf = str_len;
                buf += sizeof(unsigned short);
                memcpy(buf, str, str_len);

                buf += str_len;
            }
        }else if(lua_isuserdata(L, i)){
            LuaMessage *message_lua = (LuaMessage *)luaL_checkudata(L, i, "LuaMessage");
            if(message_lua == NULL){
                LOG_ERROR("checkuserdata is null");
                return 0;
            }
            google::protobuf::Message *message = message_lua->message;
            if(message == NULL){
                LOG_ERROR("message is null");
                return 0;
            }
            const char *msg_name = message->GetDescriptor()->full_name().data();
            if(msg_name == NULL){
                LOG_ERROR("cmd is null");
                return 0;
            }
            int msg_name_len = strlen(msg_name);
            int msg_len = message->ByteSize();
            *(unsigned char *)buf = ARG_TYPE_PB;
            buf += sizeof(unsigned char);

            *(unsigned int *)buf = (sizeof(unsigned short) + msg_name_len + msg_len);
            buf += sizeof(unsigned int);

            *(unsigned short *)buf = msg_name_len;
            buf += sizeof(unsigned short);

            memcpy(buf, msg_name, msg_name_len);
            buf += msg_name_len;

            char * buf_end = (char *)message->SerializeWithCachedSizesToArray((google::protobuf::uint8 *)buf);
            if(buf_end - buf != msg_len){
                LOG_ERROR("serialize fail %d/%d msg_name:%s\n", buf_end - buf, msg_len, msg_name);
                return 0;
            }
            buf += msg_len;

            if(message->ByteSize() < 1024){
                LOG_DEBUG("%s", message->DebugString().data());
            }
            //printf("msg size %d\n", msg_len);

        }else if(lua_istable(L, i)){
            *(unsigned char *)buf = ARG_TYPE_TABLE;
            buf += sizeof(unsigned char);
            lua_pushvalue(L, i);
            char *str = json_encode(L);
            lua_pop(L, 1);
            if(str == NULL){
                return 0;
            }
            LOG_DEBUG("%s", str);
            int str_len = strlen(str);

            *(unsigned int *)buf = str_len;
            buf += sizeof(unsigned int);
            memcpy(buf, str, str_len);
            buf += str_len;
            //释放 
            free(str);
        }else if(lua_isnil(L, i)){
            *(unsigned char *)buf = ARG_TYPE_NIL;
            buf += sizeof(unsigned char);

            LOG_DEBUG("nil");
        }else {
            LOG_ERROR("unsupport");
            return 0;
        }
    }
    *(unsigned char *)buf = ARG_TYPE_END;
    buf += sizeof(unsigned char);
    return 1;
}

static int post_action(lua_State *L){

    struct timeval t1;
    gettimeofday(&t1, NULL);
    if(!lua_isuserdata(L, 1)){
        LOG_ERROR("userdata");
        return 0;
    }
    RMIMessage* rmi_message = (RMIMessage *)lua_touserdata(L, 1);
    if(rmi_message == NULL){
        LOG_ERROR("fail");
        return 0;
    }
    int linefd = rmi_message->linefd;
    if(linefd == 0){
        LOG_ERROR("is nil");
        return 0;
    }
    Line *line = line_from_fd(linefd);
    int portfd = line->portfd;
    Port *port = port_from_fd(portfd);
    if(port == NULL){
        LOG_ERROR("line is null %s.%s", rmi_message->mod_name, rmi_message->action_name);
        return 0;
    }
    int buffd = line->buf_write;
    if(line_is_close(linefd)){
        LOG_ERROR("line is close %s %s.%s", port->name, rmi_message->mod_name, rmi_message->action_name);
        return 0;
    }

    LOG_MSG("[POST_SEND]%s %s.%s", port->name, rmi_message->mod_name, rmi_message->action_name);
    int body_len = rmi_strlen(L, rmi_message);
    if(body_len == 0){
        LOG_ERROR("strlen fail");
        return 0;
    }
    int plen = body_len + sizeof(unsigned int);
    char *buf = bbuf_alloc(buffd, plen);
    if(buf == NULL){
        LOG_ERROR("alloc fail");
        return 0;
    }
    if(rmi_encode(L, rmi_message, buf, plen) == 0){
        LOG_ERROR("encode fail");
        return 0;
    }
    bbuf_append(buffd, buf, plen);

    line_notify_writable(linefd);


    struct timeval t2;
    gettimeofday(&t2, NULL);
    LOG_MSG("[POST_SEND_END] usec:%d body_len:%d", time_diff(&t1, &t2), body_len);

    lua_pushinteger(L, 1);
    return 1;
}
int post_action_index(lua_State *L){
    if(lua_isuserdata(L, 1) && lua_isstring(L, 2)){
        RMIMessage* rmi_message = (RMIMessage *)lua_touserdata(L, 1);
        const char *action_name = (const char *)lua_tostring(L, 2);
        strcpy(rmi_message->action_name, action_name);

        lua_pushlightuserdata(L, rmi_message);
        luaL_getmetatable(L, "PostAction");
        lua_setmetatable(L, -2);
        return 1;
    }
    lua_pushnil(L);
    return 1;
}
static int post_modname_index(lua_State *L){
    if(lua_isuserdata(L, 1) && lua_isstring(L, 2)){
        RMIMessage* rmi_message = (RMIMessage *)lua_touserdata(L, 1);
        const char *mod_name = (const char *)lua_tostring(L, 2);
        strcpy(rmi_message->mod_name, mod_name);
        lua_pushlightuserdata(L, rmi_message);
        luaL_getmetatable(L, "PostActionIndex");
        lua_setmetatable(L, -2);
        return 1;
    }
    lua_pushnil(L);
    return 1;
}

static int Post(lua_State *L){
    int linefd = lua_tointeger(L, 1);
    g_rmi_message.linefd = linefd;
    lua_pushlightuserdata(L, &g_rmi_message);
    luaL_getmetatable(L, "PostModNameIndex");
    lua_setmetatable(L, -2);
    return 1;
}

static int NewPost(lua_State *L){
    if(lua_isnumber(L, 1)){
        int linefd = (int)lua_tointeger(L, 1);
        RMIMessage *rmi_message = (RMIMessage *)lua_newuserdata(L, sizeof(RMIMessage));
        if(rmi_message == NULL){
            return 0;
        }
        rmi_message->linefd = linefd;
        luaL_getmetatable(L, "PostModNameIndex");
        lua_setmetatable(L, -2);
        return 1;
    }
    return 0;
}

static char *get_buf;
static int get_buf_len;

static int get_action(lua_State *L){

    if(!lua_isuserdata(L, 1)){
        LOG_ERROR("userdata");
        return 0;
    }
    RMIMessage* rmi_message = (RMIMessage *)lua_touserdata(L, 1);
    if(rmi_message == NULL){
        LOG_ERROR("fail");
        return 0;
    }
    int linefd = rmi_message->linefd;

    if(line_is_close(linefd)){
        LOG_ERROR("line is close");
        return 0;
    }

    Line *line = line_from_fd(linefd);
    int sockfd = line->sockfd;

    int body_len = rmi_strlen(L, rmi_message);
    if(body_len == 0){
        LOG_ERROR("strlen fail");
        return 0;
    }
    int plen = body_len + sizeof(unsigned int);
    if(get_buf_len < plen){
        if(get_buf != NULL){
            free(get_buf);
        }
        get_buf = (char *)malloc(plen);
        if(get_buf == NULL){
            LOG_ERROR("malloc fail");
            return 0;
        }
        get_buf_len = plen;
    }
    char *buf = get_buf;
    if(buf == NULL){
        LOG_ERROR("alloc fail");
        return 0;
    }
    if(rmi_encode(L, rmi_message, buf, plen) == 0){
        LOG_ERROR("encode fail");
        return 0;
    }
    LOG_MSG("[GET] %s.%s body_len:%d", rmi_message->mod_name, rmi_message->action_name, body_len);


    setblock(sockfd);
    struct timeval tv;tv.tv_sec = rmi_message->timeout_sec; tv.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET,SO_RCVTIMEO, (char *)&tv,sizeof(tv));
    setsockopt(sockfd, SOL_SOCKET,SO_SNDTIMEO, (char *)&tv,sizeof(tv));
    int remain = plen;
    while(remain > 0){
        int ir = send(sockfd, buf, remain, 0);
        if(ir <= 0){
            LOG_ERROR("send fail %s", strerror(errno));
            setnonblock(sockfd);
            lua_pushstring(L, strerror(errno));
            return 1;
        }
        remain -= ir;
        buf += ir;
    }
    //接收一个json
    int ir = recv(sockfd, (char *)&plen, 4, 0); 
    if(ir < 4){
        LOG_ERROR("recv fail %s", strerror(errno));
        setnonblock(sockfd);
        lua_pushstring(L, strerror(errno));
        return 1;
    }
    if(get_buf_len < plen + 1){
        if(get_buf != NULL){
            free(get_buf);
        }
        get_buf = (char *)malloc(plen + 1);
        if(get_buf == NULL){
            LOG_ERROR("malloc fail");
            setnonblock(sockfd);
            lua_pushstring(L, strerror(errno));
            return 1;
        }
        get_buf_len = plen + 1;
    }
    buf = get_buf;
    remain = plen;
    while(remain > 0){
        int ir = recv(sockfd, buf, remain, 0);
        if(ir <= 0){
            LOG_ERROR("recv fail %s", strerror(errno));
            setnonblock(sockfd);
            lua_pushstring(L, strerror(errno));
            return 1;
        }
        remain -= ir;
        buf += ir;
    }
    LOG_LOG("REPLY");
    get_buf[plen] = 0;
    json_decode(L, get_buf);
    setnonblock(sockfd);
    return 1;
}

int get_action_index(lua_State *L){
    if(lua_isuserdata(L, 1) && lua_isstring(L, 2)){
        RMIMessage* rmi_message = (RMIMessage *)lua_touserdata(L, 1);
        const char *action_name = (const char *)lua_tostring(L, 2);
        strcpy(rmi_message->action_name, action_name);

        lua_pushlightuserdata(L, rmi_message);
        luaL_getmetatable(L, "GetAction");
        lua_setmetatable(L, -2);
        return 1;
    }
    lua_pushnil(L);
    return 1;
}
static int get_modname_index(lua_State *L){
    if(lua_isuserdata(L, 1) && lua_isstring(L, 2)){
        RMIMessage* rmi_message = (RMIMessage *)lua_touserdata(L, 1);
        const char *mod_name = (const char *)lua_tostring(L, 2);
        strcpy(rmi_message->mod_name, mod_name);
        lua_pushlightuserdata(L, rmi_message);
        luaL_getmetatable(L, "GetActionIndex");
        lua_setmetatable(L, -2);
        return 1;
    }
    lua_pushnil(L);
    return 1;
}

static int Get(lua_State *L){
    int linefd = lua_tointeger(L, 1);
    int timeout_sec = (int)lua_tointeger(L, 2);
    g_rmi_message.linefd = linefd;
    g_rmi_message.timeout_sec = timeout_sec;
    lua_pushlightuserdata(L, &g_rmi_message);
    luaL_getmetatable(L, "GetModNameIndex");
    lua_setmetatable(L, -2);
    return 1;
}

static int Reply(lua_State *L){
    int linefd = (int)lua_tointeger(L, 1);
    Line *line = line_from_fd(linefd);
    
    if(line == NULL){
        LOG_ERROR("line is bad");
        return 0;
    }

    char *str = json_encode(L);
    int str_len = strlen(str);

    int buffd = line->buf_write;
    char *buf = bbuf_alloc(buffd, str_len + 4);
    if(buf == NULL){
        free(str);
        LOG_ERROR("alloc fail");
        return 0;
    }
    *(int *)buf = str_len;
    buf += sizeof(int);
    memcpy(buf, str, str_len);
    bbuf_append(buffd, buf, str_len + 4);

    line_notify_writable(linefd);
    free(str);

    return 1;
}

static int broadcast_action(lua_State *L){
    struct timeval t1;
    gettimeofday(&t1, NULL);
    if(!lua_isuserdata(L, 1)){
        LOG_ERROR("userdata");
        return 0;
    }
    RMIMessage* rmi_message = (RMIMessage *)lua_touserdata(L, 1);
    if(rmi_message == NULL){
        LOG_ERROR("fail");
        return 0;
    }
    int portfd = rmi_message->portfd;
    Port *port = port_from_fd(portfd);
    int body_len = rmi_strlen(L, rmi_message);
    if(body_len == 0){
        LOG_ERROR("strlen fail");
        return 0;
    }
    LOG_MSG("[BROADCAST]%s.%s.%s body_len:%d", port->name, rmi_message->mod_name, rmi_message->action_name, body_len);
    for(int linefd = 1; linefd < MAX_LINE; linefd++){
        Line *line = line_from_fd(linefd);
        if(line->used == 0){
            continue;
        }
        if(line->status != LINE_STATUS_CONNECTED){
            continue;
        }
        if(line->portfd != portfd){
            continue;
        }
        int buffd = line->buf_write;

        int plen = body_len + sizeof(unsigned int);
        char *buf = bbuf_alloc(buffd, plen);
        if(buf == NULL){
            LOG_ERROR("alloc fail");
            return 0;
        }
        if(rmi_encode(L, rmi_message, buf, plen) == 0){
            LOG_ERROR("encode fail");
            return 0;
        }
        bbuf_append(buffd, buf, plen);

        line_notify_writable(linefd);
    }
    struct timeval t2;
    gettimeofday(&t2, NULL);
    LOG_MSG("[BROADCAST_END] usec:%d", time_diff(&t1, &t2));
    lua_pushinteger(L, 1);
    return 1;
}
static int broadcast_action_index(lua_State *L){
    if(lua_isuserdata(L, 1) && lua_isstring(L, 2)){
        RMIMessage* rmi_message = (RMIMessage *)lua_touserdata(L, 1);
        const char *action_name = (const char *)lua_tostring(L, 2);
        strcpy(rmi_message->action_name, action_name);
        lua_pushlightuserdata(L, rmi_message);
        luaL_getmetatable(L, "BroadcastAction");
        lua_setmetatable(L, -2);
        return 1;
    }
    lua_pushnil(L);
    return 1;
}
static int broadcast_modname_index(lua_State *L){
    if(lua_isuserdata(L, 1) && lua_isstring(L, 2)){
        RMIMessage* rmi_message = (RMIMessage *)lua_touserdata(L, 1);
        const char *mod_name = (const char *)lua_tostring(L, 2);
        strcpy(rmi_message->mod_name, mod_name);
        lua_pushlightuserdata(L, rmi_message);
        luaL_getmetatable(L, "BroadcastActionIndex");
        lua_setmetatable(L, -2);
        return 1;
    }
    lua_pushnil(L);
    return 1;
}


/*
 * 广播
 */
static int Broadcast(lua_State *L){
    if(lua_isnumber(L, 1)){
        int portfd = (int)lua_tointeger(L, 1);
        RMIMessage *rmi_message = (RMIMessage *)lua_newuserdata(L, sizeof(RMIMessage));
        if(rmi_message == NULL){
            return 0;
        }
        rmi_message->portfd = portfd;
        luaL_getmetatable(L, "BroadcastModNameIndex");
        lua_setmetatable(L, -2);
        return 1;
    }
    return 0;
}
static int New(lua_State *L)
{
    if (lua_gettop(L) == 0)
    {
        int portfd = port_new();
        if(portfd > 0){
            port_set_cb_packet(portfd, rmi_port_cb_packet);
        }
        lua_pushnumber(L, portfd);
        return 1;
    }
    lua_pushnumber(L, -1);
    return 1;
}
static luaL_Reg lua_lib[] = 
{
    {"New", New},
    {"Broadcast", Broadcast},
    {"POST", NewPost},
    {NULL, NULL}
};

int rmi_port_init(lua_State *L){

	luaL_register(L, "RMIPort", (luaL_Reg*)lua_lib);
    
    lua_pushcfunction(L, Post);
    lua_setglobal(L, "POST");

    lua_pushcfunction(L, Get);
    lua_setglobal(L, "GET");

    lua_pushcfunction(L, Reply);
    lua_setglobal(L, "REPLY");

    luaL_newmetatable(L, "PostModNameIndex");
    lua_pushstring(L, "__index");
    lua_pushcfunction(L, post_modname_index);
    lua_settable(L, -3);

    luaL_newmetatable(L, "PostActionIndex");
    lua_pushstring(L, "__index");
    lua_pushcfunction(L, post_action_index);
    lua_settable(L, -3);

    luaL_newmetatable(L, "PostAction");
    lua_pushstring(L, "__call");
    lua_pushcfunction(L, post_action);
    lua_settable(L, -3);


    luaL_newmetatable(L, "GetModNameIndex");
    lua_pushstring(L, "__index");
    lua_pushcfunction(L, get_modname_index);
    lua_settable(L, -3);

    luaL_newmetatable(L, "GetActionIndex");
    lua_pushstring(L, "__index");
    lua_pushcfunction(L, get_action_index);
    lua_settable(L, -3);

    luaL_newmetatable(L, "GetAction");
    lua_pushstring(L, "__call");
    lua_pushcfunction(L, get_action);
    lua_settable(L, -3);


    luaL_newmetatable(L, "BroadcastModNameIndex");
    lua_pushstring(L, "__index");
    lua_pushcfunction(L, broadcast_modname_index);
    lua_settable(L, -3);

    luaL_newmetatable(L, "BroadcastActionIndex");
    lua_pushstring(L, "__index");
    lua_pushcfunction(L, broadcast_action_index);
    lua_settable(L, -3);

    luaL_newmetatable(L, "BroadcastAction");
    lua_pushstring(L, "__call");
    lua_pushcfunction(L, broadcast_action);
    lua_settable(L, -3);
    return 1;
}

