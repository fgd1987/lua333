#include <netdb.h>
#include <unistd.h>
#include <execinfo.h>
#include <time.h>
#include <string.h>
#include <sys/time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <errno.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>


static int lfork(lua_State *L){
	int ir = fork();
    lua_pushinteger(L, ir);
    return 1;
}

static int lgetpid(lua_State *L){
	int ir = getpid();
    lua_pushinteger(L, ir);
    return 1;
}

static int lsetsid(lua_State *L){
	int ir = setsid();
    lua_pushinteger(L, ir);
    return 1;
}

static int ltime(lua_State *L){
    int t = time(NULL);
    lua_pushinteger(L, t);
    return 1;
}

static int lgetcwd(lua_State *L){
    char buf[128];
    buf[0] = 0;
	getcwd(buf, sizeof(buf));
    lua_pushstring(L, buf); 
    return 1;
}

static int lchdir(lua_State *L){
	const char *dir = (const char *)lua_tostring(L, 1);
	int error = chdir(dir);
    lua_pushinteger(L, error);
    return 1;
}

/*
 * @arg1 number 毫秒数
 * @return bool
 */
static int lsleepmsec(lua_State *L){
	if (lua_gettop(L) == 1 && lua_isnumber(L, 1)){
		int t = (int)lua_tonumber(L, 1);
        int sec = t / 1000;
        int nsec = (t % 1000) * 1000000;
        struct timespec slptm;
        slptm.tv_sec = sec;
        slptm.tv_nsec = nsec;
        nanosleep(&slptm, NULL);
        return 0;
	}
    return 0;
}

static int lsleep(lua_State *L){
    int seconds;
    int error;
    seconds = (int)lua_tointeger(L, 1);
    error = sleep(seconds);
    lua_pushinteger(L, error);
    return 1;
}

static int lwaitpid(lua_State *L){
    int status;
    pid_t pid;
    int options;
    pid = (int)lua_tointeger(L, 1);
    status = (int)lua_tointeger(L, 2);
    options = (int)lua_tointeger(L, 3);
    pid = waitpid(pid, &status, options);
    lua_pushinteger(L, pid);
    lua_pushinteger(L, status);
    return 2;
}

static int lmkdirs(lua_State *L){
    int i;
	if (lua_gettop(L) == 1 && lua_isstring(L, 1)){
        size_t dir_len = 0;
		char *dir = (char *)lua_tolstring(L, 1, &dir_len);
        for(i = 0; i < dir_len; i++){
            if(dir[i] == '/'){
                char c = dir[i];
                dir[i] = 0;
                if(access(dir, 0)){
                    mkdir(dir, 0777);    
                }
                dir[i] = c;
            }
        }
        if(access(dir, 0)){
            mkdir(dir, 0777);    
        }
        lua_pushboolean(L, 1);
        return 1;
	}
    return 0;
}

static int lmkdir(lua_State *L){
	if (lua_gettop(L) == 1 && lua_isstring(L, 1)){
		const char *dir = (const char *)lua_tostring(L, 1);
        if(!mkdir(dir, 0777)){
            lua_pushboolean(L, 1);
            return 1;
        }
	}
    return 0;
}

static int lexists(lua_State *L){
    int amode;
	const char *dir;
    dir = (const char *)lua_tostring(L, 1);
    amode = (int)lua_tointeger(L, 2);
    if(!access(dir, amode)) {
        lua_pushboolean(L, 1);
        return 1;
    }
    return 0;
}


static int lremove(lua_State *L){
	const char *filepath = (const char *)lua_tostring(L, 1);
    if(!remove(filepath)) {
        lua_pushboolean(L, 1);
        return 1;
    }
    return 0;
}

static int lrename(lua_State *L){
    int error;
	const char *src = (const char *)lua_tostring(L, 1);
	const char *dst = (const char *)lua_tostring(L, 2);
    error = rename(src, dst);
    lua_pushinteger(L, error);
    return 1;
}

/*
 * 
 * 返回值 {{type = 'file|dir', name = ''}, ...}
 */
static int llistdir(lua_State *L){
	if (lua_gettop(L) == 1 && lua_isstring(L, 1)){
		const char *dir_name = (const char *)lua_tostring(L, 1);
        struct dirent *ent;
        DIR *dir = opendir(dir_name);
        if(dir == NULL){
            return 0;
        }
        lua_newtable(L);
        int idx = 1;
        while((ent = readdir(dir)) != NULL){
            if(ent->d_type & DT_DIR || ent->d_type & DT_REG){
                if(strcmp(ent->d_name, ".") == 0){
                    continue;
                }
                if(strcmp(ent->d_name, "..") == 0){
                    continue;
                }
                lua_pushnumber(L, idx++);

                lua_newtable(L);
                lua_pushstring(L, "type");
                if(ent->d_type & DT_DIR){
                    lua_pushstring(L, "dir");
                }
                else{
                    lua_pushstring(L, "file");
                }
                lua_settable(L, -3);

                lua_pushstring(L, "name");
                lua_pushstring(L, ent->d_name);
                lua_settable(L, -3);

                lua_settable(L, -3);
            }
        }
        closedir(dir);
        return 1;
	}
    return 0;
}

static int lstrerror(lua_State *L){
    lua_pushstring(L, strerror(errno));
    return 1;
}

static int lerrno(lua_State *L){
    lua_pushinteger(L, errno);
    return 1;
}

static int ltest(lua_State *L){
    printf("test\n");
    //lua_pushinteger(L, 1);
    lua_newtable(L);
    return 1;
}

static int lclose(lua_State *L){
    int fd;
    int error;
    fd = (int)lua_tonumber(L, 1);
    error = close(fd);
    lua_pushinteger(L, error);
    return 1;
}

static luaL_Reg lua_lib[] ={
    {"test", ltest},
    {"errno", lerrno},
    {"strerror", lstrerror},
    {"fork", lfork},
    {"getpid", lgetpid},
    {"setsid", lsetsid},
    {"chdir", lchdir},
    {"getcwd", lgetcwd},
    {"listdir", llistdir},
    {"mkdir", lmkdir},
    {"mkdirs", lmkdirs},
    {"exists", lexists},
    {"remove", lremove},
    {"close", lclose},
    {"rename", lrename},
    {"time", ltime},
    {"waitpid", lwaitpid},
    {"sleep", lsleep},
    {"sleepmsec", lsleepmsec},
    {NULL, NULL}
};

int luaopen_sys(lua_State *L){
	luaL_register(L, "Sys", lua_lib);

    lua_pushstring(L, "EINPROGRESS");
    lua_pushinteger(L, EINPROGRESS);
    lua_settable(L, -3);
	return 1;
}
