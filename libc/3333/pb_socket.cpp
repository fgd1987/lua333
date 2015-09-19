#include "pb_socket.h"
#include "log.h"
#include "pbc.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>




typedef struct socket_t{
    int sockfd;
}socket_t;

static void XOR(char *str, int str_len){
    for(int i = 0; i < str_len; i++){
        str[i] = str[i] ^ 0xff;
    }
}



static char *recv_buf;
static int recv_buf_size;
#define MAX_MSG_NAME_LEN 128

static char msg_name[MAX_MSG_NAME_LEN];

static int Close(lua_State *L){
    if(lua_isuserdata(L, 1)){
        socket_t *self = (socket_t *)lua_touserdata(L, 1);
        close(self->sockfd);
        return 0;
    }
    return 0;
}
static int Recv(lua_State *L){
    socket_t *self = (socket_t *)lua_touserdata(L, 1);
    int sockfd = self->sockfd;
    int plen;
    int ir = recv(sockfd, (char *)&plen, 4, 0);
    if(ir < 4){
        close(sockfd);
        LOG_ERROR("recv error ir:%d %s", ir, strerror(errno));
        return 0;
    }
    int body_len = plen;
    if(recv_buf_size < body_len){
        if(recv_buf == NULL){
            free(recv_buf);
        }
        recv_buf = (char *)malloc(body_len);
        if(recv_buf == NULL){
            close(sockfd);
            LOG_ERROR("malloc fail %d", body_len);
            return 0;
        }
        recv_buf_size = body_len;
    }
    int remain = body_len;
    char *body = recv_buf;
    while(remain > 0){
        int ir = recv(sockfd, body, remain, 0);
        if(ir <= 0){
            close(sockfd);
            LOG_ERROR("%s", strerror(errno));
            return 0;
        }
        remain -= ir;
        body += ir;
    }
    body = recv_buf;
    int msg_name_len = *(unsigned short *)body;
    body += sizeof(unsigned short);
    body_len -= sizeof(unsigned short);
    if(msg_name_len >= MAX_MSG_NAME_LEN - 1){
        LOG_ERROR("reach max msg name len %d/%d", msg_name_len, MAX_MSG_NAME_LEN);
        return 0;
    }
    memcpy(msg_name, body, msg_name_len);
    XOR(msg_name, msg_name_len);
    msg_name[msg_name_len] = 0;
    body += msg_name_len;
    body_len -= msg_name_len;

    body += sizeof(unsigned int);
    body_len -= sizeof(unsigned int);

   google::protobuf::Message* message = pbc_load_msg(msg_name);
   if(message == NULL){
       LOG_ERROR("can not load msg %s", msg_name);
       return 0;
   }
   google::protobuf::io::ArrayInputStream stream(body, body_len);
   if(message->ParseFromZeroCopyStream(&stream) == 0){
       LOG_ERROR("parse fail\n");
       return 0;
   }
    
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
   return 1;
}


static char *send_buf;
static int send_buf_size;
static int Send(lua_State *L){
    if(lua_isuserdata(L, 1) && lua_isuserdata(L, 2)){
        socket_t *self = (socket_t *)lua_touserdata(L, 1);
        int sockfd = self->sockfd;
        LuaMessage *message_lua = (LuaMessage *)luaL_checkudata(L, 2, "LuaMessage");
        if(message_lua == NULL){
            LOG_ERROR("checkuserdata is null");
            return 0;
        }
        google::protobuf::Message *message = message_lua->message;
        if(message == NULL){
            LOG_ERROR("message is null");
            return 0;
        }
        const char *cmd = message->GetDescriptor()->full_name().data();
        if(cmd == NULL){
            LOG_ERROR("msg name is null");
            return 0;
        }
        int msg_name_len = strlen(cmd);
        memcpy(msg_name, cmd, msg_name_len);
        msg_name[msg_name_len] = 0;
        XOR(msg_name, msg_name_len);
        int body_len = message->ByteSize();
        int plen = 4 + msg_name_len + 2 + sizeof(int) + body_len;
        if(send_buf_size < plen){
            if(send_buf != NULL){
                free(send_buf);
                send_buf_size = 0;
            }
            send_buf = (char *)malloc(plen);
            if(send_buf == NULL){
                LOG_ERROR("malloc fail");
                return 0;
            }
            send_buf_size = plen;
        }
        char *buf = send_buf;

        *((int *)buf) = plen - 4;
        buf += 4;
        *((unsigned short *)buf) = msg_name_len;
        buf += 2;
        memcpy(buf, msg_name, msg_name_len);
        buf += msg_name_len;

        *((int *)buf) = 0;
        buf += 4;


        char * buf_end = (char *)message->SerializeWithCachedSizesToArray((google::protobuf::uint8 *)buf);
        if(buf_end - buf != body_len){
            LOG_ERROR("serialize fail %d/%d cmd:%s\n", buf_end - buf, body_len, cmd);
            return 0;
        }

        int remain = plen;
        buf = send_buf;
        while(remain > 0){
            int ir = send(sockfd, buf, remain, 0);
            if(ir <= 0){
                LOG_ERROR("send error %s", strerror(errno));
                break;
            }
            remain -= ir;
            buf += ir;
        }
        lua_pushboolean(L, true);
        return 1;
    }
    return 0;
}
static int Connect(lua_State *L){
    if(lua_gettop(L) == 2 && lua_isstring(L, 1) && lua_isnumber(L, 2)){
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if(sockfd == -1){
            LOG_ERROR("%s", strerror(errno));
            lua_pushboolean(L, false);
            lua_pushstring(L, strerror(errno));
            return 2;
        }
        const char *ip = (const char *)lua_tostring(L, 1);
        int port = (int)lua_tonumber(L, 2);
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = inet_addr(ip);
        if(inet_addr(ip) != (in_addr_t)-1){
            addr.sin_addr.s_addr = inet_addr(ip);   
        }else{
            struct hostent *hostent;
            hostent = gethostbyname(ip);
            if(hostent->h_addr_list[0] == NULL){
                LOG_ERROR("connect fail %s", ip);
                return 0;
            }
            //LOG_LOG("ip %s/n", inet_ntoa(*(struct in_addr*)(hostent->h_addr_list[0])));
            memcpy(&addr.sin_addr, (struct in_addr *)hostent->h_addr_list[0], sizeof(struct in_addr));
        } 
        socklen_t addrlen = sizeof(struct sockaddr);
        if(connect(sockfd, (struct sockaddr *)&addr, addrlen)){
            LOG_ERROR("%s", strerror(errno));
            lua_pushboolean(L, false);
            lua_pushstring(L, strerror(errno));
            return 2;
        }
        lua_pushboolean(L, true);

        socket_t *self = (socket_t *)lua_newuserdata(L, sizeof(socket_t));
        self->sockfd = sockfd;

        luaL_getmetatable(L, "PbSocketMetatable");
        lua_setmetatable(L, -2);
        return 1;
    }

    lua_pushboolean(L, false);
    lua_pushstring(L, "bad args");
    return 2;
}

static luaL_Reg lua_lib[] = 
{
    {"Connect", Connect},
    {NULL, NULL}
};

static luaL_Reg lua_metatable[] = 
{
    {"Recv", Recv},
    {"Send", Send},
    {"Close", Close},
    {NULL, NULL}
};


int pb_socket_init(lua_State *L){
	luaL_register(L, "PBSocket", (luaL_Reg*)lua_lib);

    luaL_newmetatable(L, "PbSocketMetatable");
    lua_pushstring(L, "__index");
    lua_pushvalue(L, -2);
    lua_settable(L, -3);
    
    lua_pushstring(L, "__gc");
    lua_pushcfunction(L, Close);
    lua_settable(L, -3);

    luaL_register(L, NULL, (luaL_Reg*)lua_metatable);

    return 1;
}
