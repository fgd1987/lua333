

#include "pb_port.h"
#include "port.h"
#include "pbc.h"
#include "bbuf.h"
#include "port.h"
#include "ar.h"
#include "luas.h" 


#define MAX_MSG_NAME_LEN 128
#define MAX_HEADER_LEN 256

static char msg_name[MAX_MSG_NAME_LEN];
static char recv_msg_name[MAX_MSG_NAME_LEN];

static int time_diff(struct timeval *t1, struct timeval *t2){
    int usec = (t2->tv_sec - t1->tv_sec) * 1000000 + t2->tv_usec - t1->tv_usec;
    return usec;
}

static void XOR(char *str, int str_len){
    for(int i = 0; i < str_len; i++){
        str[i] = str[i] ^ 0xff;
    }
}


static int pb_port_cb_packet(int portfd, int linefd, int bodyfd){

    Port *port = port_from_fd(portfd);
    Line *line = line_from_fd(linefd);
    if(port == NULL){
        LOG_ERROR("port is null");
        return 0;
    }
    if(line == NULL){
        LOG_ERROR("line is null");
        return 0;
    }
    char *body = ar_ptr(bodyfd);
    int body_len = ar_remain(bodyfd);

    int plen = *(int *)body + 4;
    body += sizeof(int);
    body_len -= sizeof(int);

    int msg_name_len = *(unsigned short *)body;
    body += sizeof(unsigned short);
    body_len -= sizeof(unsigned short);
    if(msg_name_len >= MAX_MSG_NAME_LEN - 1){
        LOG_ERROR("reach max msg name len %d/%d", msg_name_len, MAX_MSG_NAME_LEN);
        return 0;
    }
    memcpy(recv_msg_name, body, msg_name_len);
    recv_msg_name[msg_name_len] = 0;
    XOR(recv_msg_name, msg_name_len);

    body += msg_name_len;
    body_len -= msg_name_len;

    int seq = *(int *)body;
    body += sizeof(int);
    body_len -= sizeof(int);

    lua_State *L = luas_open();
    const char *func = port->lua_cb_packet;

    line->recv_seq = 4 * line->recv_seq + 9;
    if(line->uid != 0 && line->recv_seq != seq){
        LOG_ERROR("seq error expected %d recv %d", line->recv_seq, seq);
    }else if(func[0] != 0){
        struct timeval t1;
        gettimeofday(&t1, NULL);

        google::protobuf::Message* message = pbc_load_msg(recv_msg_name);
        if(message == NULL){
            LOG_ERROR("can not load %d msg %s", msg_name_len, recv_msg_name);
            return 0;
        }
        google::protobuf::io::ArrayInputStream stream(body, body_len);
        if(message->ParseFromZeroCopyStream(&stream) == 0){
            delete message;
            LOG_ERROR("parse fail\n");
            return 0;
        }

        LOG_MSG("[%d]RECV %s plen:%d FROM %s", line->uid, recv_msg_name, plen, port->name);

        if(message->ByteSize() < 4096){
            LOG_DEBUG("%s", message->DebugString().data());
        }

        luas_pushluafunction(func);
        //sock 
        lua_pushnumber(L, linefd);
        //cmd
        lua_pushstring(L, recv_msg_name);
        //msg
        LuaMessage *message_lua = (LuaMessage *)lua_newuserdata(L, sizeof(LuaMessage));
        if(message_lua == NULL){
            lua_pop(L, lua_gettop(L));
            delete message;
            LOG_ERROR("newuserdata null %s", recv_msg_name);
            return 0;
        }
        message_lua->message = message;
        message_lua->root_message = message_lua;
        message_lua->dirty = 0;
        luaL_getmetatable(L, "LuaMessage");
        lua_setmetatable(L, -2);
        int args_num = 3;
        if (lua_pcall(L, args_num, 1, 0) != 0){
            LOG_ERROR("error running function %s: %s", func, lua_tostring(L, -1));
        }
        struct timeval t2;
        gettimeofday(&t2, NULL);
        LOG_MSG("[%d]RECV_END %s usec:%d FROM %s", line->uid, recv_msg_name, time_diff(&t1, &t2), port->name);
        lua_pop(L, lua_gettop(L));

    }
    return 1;
}

static int New(lua_State *L){
    if (lua_gettop(L) == 0){
        int portfd = port_new();
        if(portfd > 0){
            port_set_cb_packet(portfd, pb_port_cb_packet);
            lua_pushnumber(L, portfd);
            return 1;
        }
    }
    lua_pushboolean(L, false);
    return 1;
}
static int Send(lua_State *L){

	if (lua_isnumber(L, 1) && lua_isuserdata(L, 2)){
        struct timeval t1;
        gettimeofday(&t1, NULL);
        int linefd = (int)lua_tonumber(L, 1);

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
            LOG_ERROR("cmd is null");
            return 0;
        }
        if(line_is_close(linefd)){
            LOG_ERROR("line %d is close", linefd);
            return 0;
        }

        int msg_name_len = strlen(cmd);
        memcpy(msg_name, cmd, msg_name_len);
        msg_name[msg_name_len] = 0;
        XOR(msg_name, msg_name_len);

        Line *line = line_from_fd(linefd);
        int portfd = line->portfd;
        Port *port = port_from_fd(portfd);

        int buffd = line->buf_write;
        int body_len = message->ByteSize();
        int plen = 4 + msg_name_len + 2  + sizeof(int) + body_len;
        char *buf = bbuf_alloc(buffd, plen);
        if(buf == NULL){
            LOG_ERROR("alloc fail cmd:%s\n", cmd);
            return 0;
        }

        *((int *)buf) = plen - 4;
        buf += 4;
        *((unsigned short *)buf) = msg_name_len;
        buf += 2;
        memcpy(buf, msg_name, msg_name_len);
        buf += msg_name_len;
        
        line->send_seq = 4 * line->send_seq + 9;
        *((int *)buf) = line->send_seq;
        buf += sizeof(int);

        char * buf_end = (char *)message->SerializeWithCachedSizesToArray((google::protobuf::uint8 *)buf);
        if(buf_end - buf != body_len){
            LOG_ERROR("serialize fail %d/%d cmd:%s\n", buf_end - buf, body_len, cmd);
            return 0;
        }
        //加进socket的发送队列
        bbuf_append(buffd, buf, plen);

        line_notify_writable(linefd);

        struct timeval t2;
        gettimeofday(&t2, NULL);

        LOG_MSG("[%d]SEND %s plen:%d usec:%d TO %s", line->uid, cmd, plen, time_diff(&t1, &t2), port->name)
        if(body_len < 4096){
            LOG_DEBUG("%s", message->DebugString().data());
        }
        lua_pushboolean(L, true);
        return 1;
	}
    lua_pushboolean(L, false);
    return 1;
}

static int Split(lua_State *L){
    if(lua_gettop(L) == 1 && lua_isstring(L, 1)){
        size_t strlen;
        const char *str = (const char *)lua_tolstring(L, 1, &strlen);
        int pos = 0;
        for(unsigned int i = 0; i < strlen; i++){
            if(str[i] == '.'){
                pos = i;
                break;
            }
        }
        lua_pushlstring(L, str, pos);
        lua_pushlstring(L, str + pos + 1, strlen - pos - 1);
        return 2;
    }
    return 0;
}

static luaL_Reg lua_lib[] = {
    {"New", New},
    {"Send", Send},
    {"Split", Split},
    {NULL, NULL}
};

int pb_port_init(lua_State *L){
    luaL_register(L, "PBPort", lua_lib);
    return 1;
}

