#include "sys.h"
#include "luas.h"
#include "bbuf.h"
#include "port.h"
#include "timer.h"
#include "pbc.h"
#include "evt.h"
#include "linenoise.h"
#include "utils.h"
#include <netdb.h>
#include <execinfo.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <ifaddrs.h>

#ifndef __APPLE__ 
#include <openssl/md5.h>
#else
#include "md5.h"
#endif

static int Fork(lua_State *L){
	int ir = fork();
    lua_pushinteger(L, ir);
    return 1;
}
static int GetPid(lua_State *L){
	int ir = getpid();

    lua_pushinteger(L, ir);
    return 1;
}

static int SetSid(lua_State *L){
	int ir = setsid();

    lua_pushinteger(L, ir);
    return 1;
}

static time_t s_timenow = 0;
static time_t s_record_time = 0;
static int Timenow(lua_State *L){
    if(s_timenow != 0){
        time_t t = time(NULL);
        t = s_timenow + (t - s_record_time);
        lua_pushinteger(L, t);
        return 1;
    }else{
        int t = time(NULL);
        lua_pushinteger(L, t);
        return 1;
    }
}

static int ResetTime(lua_State *L){
    if(lua_gettop(L) == 0){
        s_timenow = 0;
        lua_pushboolean(L, true);
        return 1;
    }
    lua_pushboolean(L, false);
    return 1;
}

static int SetTime(lua_State *L){
    if(lua_gettop(L) == 1 && lua_isstring(L, 1)){
        char *str = (char *)lua_tostring(L, 1);
        struct tm tm;
        strptime(str, "%Y-%m-%d-%H:%M:%S", &tm);
        s_timenow = mktime(&tm);
        s_record_time = time(NULL);
        lua_pushboolean(L, true);
        return 1;
    }
    lua_pushboolean(L, false);
    return 1;
}

static int Exit(lua_State *L){
    evt_stop();
    return 0;
}
//返回系统的微秒时间
static int TimenowUsec(lua_State *L){
    struct timeval tv; 
    struct timezone tz; 
    gettimeofday(&tv,&tz);
    lua_pushinteger(L,tv.tv_usec);
    lua_pushinteger(L,tv.tv_sec);
    return 2;
}

static int GetCwd(lua_State *L){
    char buf[128];
    buf[0] = 0;
	getcwd(buf, sizeof(buf));
    lua_pushstring(L, buf); 
    return 1;
}

static int InitDaemon(lua_State *L){
    int pid;
    pid = fork();
    if(pid){
        exit(0);
    }else if(pid < 0){
        LOG_ERROR("fork error");
        exit(1);
    }
	setsid();
    pid = fork();
    if(pid){
        exit(0);
    }else if(pid < 0){
        LOG_ERROR("fork error");
        exit(1);
    }
    //ps, 关了就不要写
	/*for(i = 0; i <=2; i++){
		close(i);
	}*/

    signal(SIGINT, SIG_IGN);

    lua_pushboolean(L, true);
    return 1;
}
static int SetStdOut(lua_State *L){
	if (lua_gettop(L) == 1 && lua_isstring(L, 1)){
		const char *filepath = (const char *)lua_tostring(L, 1);
		int fd = open(filepath, (O_RDWR | O_CREAT | O_APPEND | O_TRUNC), 0644 );
		dup2(fd, 1);
		close(fd);
        lua_pushboolean(L, true);
        return 1;
	}
    lua_pushboolean(L, false);
    return 1;
}
static int ChDir(lua_State *L){
	if (lua_gettop(L) == 1 && lua_isstring(L, 1)){
		const char *dir = (const char *)lua_tostring(L, 1);
		chdir(dir);
        lua_pushboolean(L, true);
        return 1;
	}
    lua_pushboolean(L, false);
    return 1;
}
static int cmd(lua_State *L){
    if(lua_isstring(L, 1)){
		const char *str = lua_tostring(L, 1);
        //拆参数
        char buf[256];
        strcpy(buf, str);
        int len = strlen(buf);
        char *pbuf = buf;
        char *cmd = pbuf;
        char *argv[128];
        argv[0] = cmd;
        int need_blank = 0;
        int i ;
        int idx = 0;
        for(i = 0; i < len; i++){
            if(need_blank == 0 && pbuf[i] != ' '){
                argv[idx++] = pbuf + i;
                need_blank = 1;
            }else if(need_blank == 1 && pbuf[i] == ' '){
                pbuf[i] = 0;
                need_blank = 0;
            }
        }
        argv[idx] = NULL;
        int pid = fork();
        if(pid == 0){
	        execvp(cmd, argv);	
            exit(1);
        }else if(pid < 0){
        }else if(pid > 0){
            int status;
            waitpid(pid, &status, 0);
        }
        lua_pushboolean(L, true);
        return 1;
	}
    lua_pushboolean(L, false);
    return 1;
}
static int Stop(lua_State *L){
	if (lua_gettop(L) == 1 && lua_isnumber(L, 1)){
        evt_stop();
        lua_pushboolean(L, true);
        return 1;
    }
    lua_pushboolean(L, false);
    return 1;
}
static int GetHost(lua_State *L){
	if (lua_gettop(L) == 0){
        lua_newtable(L);
        
        int idx = 1;
        struct ifaddrs *ifaddrs = NULL;
        getifaddrs(&ifaddrs);
        while(ifaddrs){
            if(ifaddrs->ifa_addr->sa_family == AF_INET){
                void *addr_ptr = &((struct sockaddr_in *)ifaddrs->ifa_addr)->sin_addr;
                char ip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, addr_ptr, ip, INET_ADDRSTRLEN);
                lua_pushnumber(L, idx++);
                lua_pushstring(L, ip);
                lua_settable(L, -3);
            }
            ifaddrs = ifaddrs->ifa_next;
        }
        return 1;
	}
    lua_pushboolean(L, false);
    return 1;
}
static int NanoSleep(lua_State *L){
	if (lua_gettop(L) == 1 && lua_isnumber(L, 1)){
		int t = (int)lua_tonumber(L, 1);
        int sec = t / 1000;
        int nsec = (t % 1000) * 1000000;
        struct timespec slptm;
        slptm.tv_sec = sec;
        slptm.tv_nsec = nsec;
        nanosleep(&slptm, NULL);
        lua_pushboolean(L, true);
        return 1;
	}
    lua_pushboolean(L, false);
    return 1;
}

static int Sleep(lua_State *L){
	if (lua_gettop(L) == 1 && lua_isnumber(L, 1)){
		int t = (int)lua_tonumber(L, 1);
        sleep(t);
        lua_pushboolean(L, true);
        return 1;
	}
    lua_pushboolean(L, false);
    return 1;
}
static int waitpid(lua_State *L){
	if (lua_gettop(L) == 1 && lua_isnumber(L, 1)){
		int pid = (int)lua_tonumber(L, 1);
        int status;
        waitpid(pid, &status, 0);
        lua_pushnumber(L, status);
        return 1;
	}
    lua_pushboolean(L, false);
    return 1;
}
static int mkdir(lua_State *L){
	if (lua_gettop(L) == 1 && lua_isstring(L, 1)){
		const char *dir = (const char *)lua_tostring(L, 1);
        mkdir(dir, 0777);
        lua_pushboolean(L, true);
        return 1;
	}
    lua_pushboolean(L, false);
    return 1;
}
static int Exists(lua_State *L){
	if (lua_gettop(L) == 1 && lua_isstring(L, 1)){
		const char *dir = (const char *)lua_tostring(L, 1);
        int ir = access(dir, 0);
        lua_pushboolean(L, ir == 0);
        return 1;
	}
    lua_pushboolean(L, false);
    return 1;
}

static int close(lua_State *L){
	if (lua_gettop(L) == 1 && lua_isnumber(L, 1)){
		int fd = (int)lua_tonumber(L, 1);
        close(fd);
        lua_pushboolean(L, true);
        return 1;
	}
    lua_pushboolean(L, false);
    return 1;
}
static int Remove(lua_State *L){
	if (lua_gettop(L) == 1 && lua_isstring(L, 1)){
		const char *filepath = (const char *)lua_tostring(L, 1);
        remove(filepath);
        lua_pushboolean(L, true);
        return 1;
	}
    lua_pushboolean(L, false);
    return 1;
}
static int Rename(lua_State *L){
	if (lua_gettop(L) == 2 && lua_isstring(L, 1) && lua_isstring(L, 2)){
		const char *src = (const char *)lua_tostring(L, 1);
		const char *dst = (const char *)lua_tostring(L, 2);
        rename(src, dst);
        lua_pushboolean(L, true);
        return 1;
	}
    lua_pushboolean(L, false);
    return 1;
}
static int ListDir(lua_State *L){
	if (lua_gettop(L) == 1 && lua_isstring(L, 1)){
		const char *dir_name = (const char *)lua_tostring(L, 1);
        struct dirent *ent;
        DIR *dir = opendir(dir_name);
        lua_newtable(L);
        int idx = 1;
        if(dir == NULL){
            return 1;
        }
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
    lua_pushboolean(L, false);
    return 1;
}

static int FileMtime(lua_State *L){
    if(lua_gettop(L) == 1 && lua_isstring(L, 1)){
        const char *file_path = (const char *)lua_tostring(L, 1);
        struct stat info;
        if(!stat(file_path, &info)){
            lua_pushnumber(L, info.st_mtime);
            return 1;
        }
    }
    lua_pushnumber(L, 0);
    return 1;
}
static int ReadLine(lua_State *L){
    if(lua_gettop(L) == 1 && lua_isstring(L, 1)){
        const char *prompt = (const char *)lua_tostring(L, 1);
        char *str = linenoise(prompt);
        lua_pushstring(L, str);
        return 1;
    }
    lua_pushstring(L, "");
    return 1;
}
int HistoryAdd(lua_State *L){
    if(lua_gettop(L) == 1 && lua_isstring(L, 1)){
        const char *str = (const char *)lua_tostring(L, 1);
        linenoiseHistoryAdd(str);
        lua_pushboolean(L, true);
        return 1;
    }
    lua_pushboolean(L, false);
    return 1;
}
static int Str2Func(lua_State *L){
    if(lua_gettop(L) == 1 && lua_isstring(L, 1)){
        const char *func = (const char *)lua_tostring(L, 1);
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
    lua_pushnil(L);
    return 1;
}

static int BitOr(lua_State *L){
	if (lua_gettop(L) == 2 && lua_isnumber(L, 1) && lua_isnumber(L, 2)){
		int t1 = (int)lua_tonumber(L, 1);
		int t2 = (int)lua_tonumber(L, 2);
        int t = t1 | ((1 << t2)); 
        lua_pushnumber(L, t);
        return 1;
	}
    lua_pushboolean(L, false);
    return 1;
}


static int BitAnd(lua_State *L){
	if (lua_gettop(L) == 2 && lua_isnumber(L, 1) && lua_isnumber(L, 2)){
		int t1 = (int)lua_tonumber(L, 1);
		int t2 = (int)lua_tonumber(L, 2);
        int t = t1 & ((1 << t2)); 
        lua_pushnumber(L, t);
        return 1;
	}
    lua_pushboolean(L, false);
    return 1;
}


static int NotBit(lua_State *L){
	if (lua_gettop(L) == 2 && lua_isnumber(L, 1) && lua_isnumber(L, 2)){
		int t1 = (int)lua_tonumber(L, 1);
		int t2 = (int)lua_tonumber(L, 2);
        if(t1 & (1 << t2)){
            int t = t1 & (~(1 << t2)); 
            lua_pushnumber(L, t);
            return 1;
        }else
        {
            int t = t1 | (1 << t2);
            lua_pushnumber(L, t);
            return 1;
        }
	}
    lua_pushboolean(L, false);
    return 1;
}

static int And(lua_State *L){
	if (lua_gettop(L) == 2 && lua_isnumber(L, 1) && lua_isnumber(L, 2)){
		int t1 = (int)lua_tonumber(L, 1);
		int t2 = (int)lua_tonumber(L, 2);
        int t = t1 & t2;
        lua_pushnumber(L, t);
        return 1;
	}
    lua_pushboolean(L, false);
    return 1;
}

static int FromTimeStr(lua_State *L){
	if (lua_gettop(L) == 1 && lua_isstring(L, 1)){
		const char * str = (const char*)lua_tostring(L, 1);
        struct tm tm;
        sscanf(str, "%04d-%02d-%02d %02d:%02d:%02d", &tm.tm_year, &tm.tm_mon, &tm.tm_mday, &tm.tm_hour, &tm.tm_min, &tm.tm_sec);
        tm.tm_year -= 1900;
        tm.tm_mon -= 1;
        time_t t = mktime(&tm);
        lua_pushnumber(L, t);
        return 1;
	}
    lua_pushboolean(L, false);
    return 1;
}


static int Or(lua_State *L){
	if (lua_gettop(L) == 2 && lua_isnumber(L, 1) && lua_isnumber(L, 2)){
		int t1 = (int)lua_tonumber(L, 1);
		int t2 = (int)lua_tonumber(L, 2);
        int t = t1 | t2;
        lua_pushnumber(L, t);
        return 1;
	}
    lua_pushboolean(L, false);
    return 1;
}
static int GetWeekDay(lua_State *L){
	if (lua_gettop(L) == 0){
		time_t t1 = time(NULL);
        struct tm tm1;
        localtime_r(&t1, &tm1);
        lua_pushnumber(L, tm1.tm_wday);
        return 1;
	}
    lua_pushboolean(L, false);
    return 1;
}

static int IsSameMonth(lua_State *L){
	if (lua_gettop(L) == 1 && lua_isnumber(L, 1)){
		time_t t1 = (time_t)lua_tonumber(L, 1);
		time_t t2 = time(NULL);
        struct tm tm1;
        localtime_r(&t1, &tm1);
        struct tm tm2;
        localtime_r(&t2, &tm2);
        if(tm1.tm_year == tm2.tm_year &&
           tm1.tm_mon == tm2.tm_mon){
            lua_pushboolean(L, true);
            return 1;
        }
        lua_pushboolean(L, false);
        return 1;
	}
    lua_pushboolean(L, false);
    return 1;
}
static int IsSameWeek(lua_State *L){
	if (lua_gettop(L) == 1 && lua_isnumber(L, 1)){
		time_t t1 = (time_t)lua_tonumber(L, 1);
		time_t t2 = time(NULL);
        struct tm tm1;
        localtime_r(&t1, &tm1);
        struct tm tm2;
        localtime_r(&t2, &tm2);
        if(tm1.tm_year != tm2.tm_year){
            lua_pushboolean(L, false);
            return 1;
        }
        if(tm1.tm_yday == tm2.tm_yday){
            lua_pushboolean(L, true);
            return 1;
        }
        if(tm1.tm_yday > tm2.tm_yday && tm1.tm_yday - tm2.tm_yday <= tm1.tm_wday){
            lua_pushboolean(L, true);
            return 1;
        }
        if(tm1.tm_yday < tm2.tm_yday && tm2.tm_yday - tm1.tm_yday <= 6 - tm1.tm_wday){
            lua_pushboolean(L, true);
            return 1;
        }
        lua_pushboolean(L, false);
        return 1;
	}
    lua_pushboolean(L, false);
    return 1;
}


static int IsSameDay(lua_State *L){
	if (lua_gettop(L) == 1 && lua_isnumber(L, 1)){
		time_t t1 = (time_t)lua_tonumber(L, 1);
		time_t t2 = time(NULL);
        struct tm tm1;
        localtime_r(&t1, &tm1);
        struct tm tm2;
        localtime_r(&t2, &tm2);
        if(tm1.tm_year == tm2.tm_year &&
           tm1.tm_mon == tm2.tm_mon && 
           tm1.tm_mday == tm2.tm_mday){
            lua_pushboolean(L, true);
            return 1;
        }
        lua_pushboolean(L, false);
        return 1;
	}
    lua_pushboolean(L, false);
    return 1;
}

static int DaysAfter(lua_State *L){
    if(lua_gettop(L) == 2 && lua_isnumber(L, 1) && lua_isnumber(L, 2)){
        time_t timenow = (time_t)lua_tonumber(L, 1);
        time_t timebefore = (time_t)lua_tonumber(L, 2);
        int days = ((timenow - timenow % 86400) - (timebefore - timebefore % 86400)) / 86400;
        lua_pushnumber(L, days);
        return 1;
    }
    lua_pushboolean(L, false);
    return 1;
}
static int Md5(lua_State *L){
    if(lua_gettop(L) == 1 && lua_isstring(L, 1)){
        size_t str_len;
        const unsigned char* str = (const unsigned char*)lua_tolstring(L, 1, &str_len);
        unsigned char md[16];
        char tmp[3]={'\0'},buf[33]={'\0'};
        MD5(str, str_len, md);
        for(int i = 0; i < 16; i++){
            sprintf(tmp,"%2.2x",md[i]);
            strcat(buf,tmp);
        }
        lua_pushstring(L, buf);

        return 1;
    }else{
        lua_pushstring(L, "");
        return 1;
    }
}

//短网址
static int Shorturl(lua_State *L){
    if(lua_gettop(L) == 1 && lua_isstring(L, 1)){
        char* origstring = (char*)lua_tostring(L, 1);
        string* str = shorturl(origstring, strlen(origstring));
        lua_pushstring(L, str[0].c_str());
        delete []str;
        return 1;
    }else{
        lua_pushstring(L, "");
        return 1;
    }
}

static luaL_Reg lua_lib[] ={
    {"Fork", Fork},
    {"InitDaemon", InitDaemon},
    {"GetPid", GetPid},
    {"SetSid", SetSid},
    {"ChDir", ChDir},
    {"GetCwd", GetCwd},
    {"SetStdOut", SetStdOut},
    {"ListDir", ListDir},
    {"MkDir", mkdir},
    {"Exists", Exists},
    {"Remove", Remove},
    {"Close", close},
    {"Rename", Rename},
    {"Timenow", Timenow},
    {"FileMtime", FileMtime},
    {"Cmd", cmd},
    {"WaitPid", waitpid},
    {"ReadLine", ReadLine},
    {"Str2Func", Str2Func},
    {"Sleep", Sleep},
    {"NanoSleep", NanoSleep},
    {"GetHost", GetHost},
    {"Stop", Stop},


    {"FromTimeStr", FromTimeStr},
    //或运算符
    {"Or", Or},
    //与运算符
    {"And", And},
    {"NotBit", NotBit},
    {"BitAnd", BitAnd},
    {"BitOr", BitOr},
    //是否同一天
    {"IsSameDay", IsSameDay},
    //是否同一个星期
    {"IsSameWeek", IsSameWeek},
    //是否同一个月
    {"IsSameMonth", IsSameMonth},
    {"DaysAfter", DaysAfter},
    {"GetWeekDay", GetWeekDay},
    {"SetTime", SetTime},
    {"ResetTime", ResetTime},
    {"HistoryAdd", HistoryAdd},
    //md5
    {"Md5", Md5},
    //长string转成短str
    {"Shorturl", Shorturl},
    {"TimenowUsec",TimenowUsec},
    {"Exit", Exit},
    {NULL, NULL}
};
int sys_init(lua_State *L, int argc, char **argv){
	luaL_register(L, "SYS", (luaL_Reg*)lua_lib);

    lua_pushstring(L, "Argc");
    lua_pushinteger(L, argc);
    lua_settable(L, -3);

    lua_pushstring(L, "Arg");
    lua_newtable(L);
    for(int i = 0; i < argc; i++){
        lua_pushinteger(L, i+1);
        lua_pushstring(L, argv[i]);
        lua_settable(L, -3);
    }
    lua_settable(L, -3);


	return 1;
}

