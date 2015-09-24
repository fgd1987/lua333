

#include "os.h"
extern "C"{
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

int evt_init(lua_State *L);
int evt_run();
int evt_destory();

int evt_stop();


struct aeEventLoop* evt_loop();

