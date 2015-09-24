

#include "raw_port.h"
#include "port.h"
#include "bbuf.h"
#include "port.h"
#include "luas.h" 


static int New(lua_State *L){
    if (lua_gettop(L) == 0){
        int portfd = port_new();
        if(portfd > 0){
            port_set_raw(portfd);
            lua_pushnumber(L, portfd);
            return 1;
        }
    }
    lua_pushboolean(L, false);
    return 1;
}
static int Send(lua_State *L){

	if (lua_isnumber(L, 1) && lua_isstring(L, 2)){
        int linefd = (int)lua_tonumber(L, 1);
        size_t str_len;
        char *str = (char *)lua_tolstring(L, 2, &str_len);
        Line *line = line_from_fd(linefd);
        int buffd = line->buf_write;

        char *buf = bbuf_alloc(buffd, str_len);
        if(buf == NULL){
            LOG_ERROR("alloc fail");
            return 0;
        }
        memcpy(buf, str, str_len);
        //加进socket的发送队列
        bbuf_append(buffd, buf, str_len);

        line_notify_writable(linefd);

        lua_pushboolean(L, true);
        return 1;
	}
    lua_pushboolean(L, false);
    return 1;
}


static luaL_Reg lua_lib[] = {
    {"New", New},
    {"Send", Send},
    {NULL, NULL}
};

int raw_port_init(lua_State *L){
    luaL_register(L, "RawPort", lua_lib);
    return 1;
}

