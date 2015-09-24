
#include "log.h"
#include "bbuf.h"
#include "rbuf.h"
#include "port.h"
#include "timer.h"
#include "pbc.h"
#include "evt.h"

static const char *s_level_msg[] = {"ERROR", "WARNING", "LOG", "MSG", "DEBUG", "STAT"};
 
static int s_log_flag = 0xffffffff;


static int s_error_counter = 0;
static int s_warning_counter = 0;

static int s_log_file = 0;
static int s_log_file_counter = 0;
static int s_log_file_max_linenum = 10000;
static char s_log_file_name[FILENAME_MAX];
static char s_log_file_dir[FILENAME_MAX];

static FILE *s_stat_file = NULL;
static int s_stat_file_counter = 0;
static int s_stat_file_max_linenum = 10000;
static char s_stat_file_name[FILENAME_MAX];
static char s_stat_file_dir[FILENAME_MAX];


static int log_vprint(int level, const char *fmt, va_list ap);
static int stat_vprint(int level, const char *fmt, va_list ap);
static void rotate_stat_file();
static void rotate_log_file(int force);


int log_stat(const char *fmt, ...){
    va_list   args;
    va_start(args, fmt);
    stat_vprint(STAT, fmt, args);
    va_end(args);
    return 1;
}
int log_msg(const char *fmt, ...){
    va_list   args;
    va_start(args, fmt);
    log_vprint(MSG, fmt, args);
    va_end(args);
    return 1;
}
int log_debug(const char *fmt, ...){
    va_list   args;
    va_start(args, fmt);
    log_vprint(DEBUG, fmt, args);
    va_end(args);
    return 1;
}
int log_log(const char *fmt, ...){
    va_list   args;
    va_start(args, fmt);
    log_vprint(LOG, fmt, args);
    va_end(args);
    return 1;
}
int log_warning(const char *fmt, ...){
    s_warning_counter++;
    va_list   args;
    va_start(args, fmt);
    log_vprint(WARNING, fmt, args);
    va_end(args);
    return 1;
}
int log_error(const char *fmt, ...){
    s_error_counter++;
    va_list   args;
    va_start(args, fmt);
    log_vprint(ERROR, fmt, args);
    va_end(args);
    return 1;
}

static int stat_vprint(int level, const char *fmt, va_list ap){

    if(s_stat_file != NULL){
        rotate_stat_file();
        time_t t = time(NULL);
        struct tm *tm = localtime(&t);

        fprintf(s_stat_file, "[%04d-%02d-%02d %02d:%02d:%02d][STAT] ", 
                tm->tm_year + 1900, 
                tm->tm_mon + 1, 
                tm->tm_mday, 
                tm->tm_hour, 
                tm->tm_min, 
                tm->tm_sec);
        vfprintf(s_stat_file, fmt, ap);
        fprintf(s_stat_file, "\n");
        fflush(s_stat_file);
    }
   
    return 1;
}

static int log_vprint(int level, const char *fmt, va_list ap){
	if(!(s_log_flag & (1 << level))){
		return 0;
	}
    rotate_log_file(0);
    struct timeval t;
    gettimeofday(&t, NULL);
    struct tm *tm = localtime(&t.tv_sec);
 
    const char *level_msg = s_level_msg[level];
    printf("[%04d-%02d-%02d %02d:%02d:%02d.%06ld][%s]", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, t.tv_usec, level_msg);
    vprintf(fmt, ap);
    printf("\n");
    fflush(stdout);
   
    return 1;
}

static int log_print(int level, const char *str, int str_len){
	if(!(s_log_flag & (1 << level))){
		return 0;
	}
    rotate_log_file(0);
    struct timeval t;
    gettimeofday(&t, NULL);
    struct tm *tm = localtime(&t.tv_sec);
 
    const char *level_msg = s_level_msg[level];
    printf("[%04d-%02d-%02d %02d:%02d:%02d.%06ld][%s]", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, t.tv_usec, level_msg);
    fwrite(str, 1, str_len, stdout);
    printf("\n");
    fflush(stdout);
   
    return 1;
}

static void rotate_log_file(int force){
    if(s_log_file != 0){
        if(force || s_log_file_counter++ > s_log_file_max_linenum){
            char file_path[FILENAME_MAX];
            time_t t = time(NULL);
            struct tm *tm = localtime(&t);
            if(tm == NULL){
                LOG_ERROR("localtime fail");
            }
            sprintf(file_path, "%s/%s_%04d_%02d_%02d_%02d_%02d_%02d", 
                s_log_file_dir,
                s_log_file_name, 
                tm->tm_year + 1900, 
                tm->tm_mon + 1, 
                tm->tm_mday, 
                tm->tm_hour, 
                tm->tm_min, 
                tm->tm_sec);
            rename(s_log_file_name, file_path);
        
            int fd = open(s_log_file_name, O_WRONLY | O_CREAT | O_TRUNC, 0600);
            if(fd == -1){
                s_log_file_counter = 0;
                LOG_ERROR("open file fail %s %s", s_log_file_name, strerror(errno));
                return;
            }
            dup2(fd, 1);
            close(fd);
            s_log_file_counter = 0;
        }
    }
}

static void rotate_stat_file(){
    if(s_stat_file_counter++ > s_stat_file_max_linenum){
        if(s_stat_file != NULL){
            fclose(s_stat_file);
        }
        char file_path[FILENAME_MAX];
        time_t t = time(NULL);
        struct tm *tm = localtime(&t);
        if(tm == NULL){
            LOG_ERROR("localtime fail");
        }
        sprintf(file_path, "%s/%s_%04d_%02d_%02d_%02d_%02d_%02d", 
            s_stat_file_dir,
            s_stat_file_name, 
            tm->tm_year + 1900, 
            tm->tm_mon + 1, 
            tm->tm_mday, 
            tm->tm_hour, 
            tm->tm_min, 
            tm->tm_sec);
        rename(s_stat_file_name, file_path);
        s_stat_file = fopen(s_stat_file_name, "w+");
        if(s_stat_file == NULL){
            LOG_ERROR("open file fail %s %s", s_stat_file_name, strerror(errno));
            return;
        }
        s_stat_file_counter = 0;
    }
}

static int OpenLevel(lua_State *L){
	if (lua_gettop(L) == 1 && lua_isnumber(L, 1)){
        int level = (int)lua_tonumber(L, 1);

        s_log_flag |= 1 << level;
        lua_pushnumber(L, 1);
        return 1;
	}

    lua_pushnumber(L, 0);
    return 1;
}


static int CloseAll(lua_State *L){
	if (lua_gettop(L) == 0){

        s_log_flag = 0;

        lua_pushnumber(L, 1);
        return 1;
	}
    lua_pushnumber(L, 0);
    return 1;
}
static int CloseLevel(lua_State *L){
	if (lua_gettop(L) == 1 && lua_isnumber(L, 1)){
        int level = (int)lua_tonumber(L, 1);

        s_log_flag &= ~(1 << level);

        lua_pushnumber(L, 1);
        return 1;
	}
    lua_pushnumber(L, 0);
    return 1;
}
static int log_stat_timer(int msgid, void *userdata){
    bbuf_stat();
    rbuf_stat();
    port_stat();
    pbc_stat();
    timer_stat();
    LOG_STAT("==========LOG==========");
    LOG_STAT("error:(%d), warning:(%d)", s_error_counter, s_warning_counter);
    
    LOG_STAT("==========LUA==========");
    LOG_STAT("memory:(%dk) stack top(%d)", lua_gc(L, LUA_GCCOUNT, 0), lua_gettop(L));

    LOG_STAT("==========MEM==========");
    //LOG_STAT("used:%dk rss:%dk", zmalloc_used_memory()/1024, zmalloc_get_rss()/1024);

    return 1;
}

static int LogStat(lua_State *L){
	if (lua_gettop(L) == 4 && lua_isstring(L, 1) && lua_isnumber(L, 2) && lua_isnumber(L, 3) && lua_isstring(L, 4)){
        strcpy(s_stat_file_name, (const char *)lua_tostring(L, 1));
        s_stat_file_max_linenum = (int)lua_tonumber(L, 2);
        int ms = (int)lua_tonumber(L, 3);
        strcpy(s_stat_file_dir, (const char *)lua_tostring(L, 4));
        timer_add_callback(log_stat_timer, ms, NULL);
        s_stat_file = fopen(s_stat_file_name, "w+");
        if(s_stat_file == NULL){
            LOG_ERROR("open file fail %s %s", s_stat_file_name, strerror(errno));
            lua_pushboolean(L, false);
            return 1;
        }else{
            lua_pushboolean(L, true);
            return 1;
        }
	}
    lua_pushboolean(L, false);
    return 1;
}

static int LogFile(lua_State *L){
	if (lua_gettop(L) == 3 && lua_isstring(L, 1) && lua_isnumber(L, 2) && lua_isstring(L, 3)){
        strcpy(s_log_file_name, (const char *)lua_tostring(L, 1));
        s_log_file_max_linenum = (int)lua_tonumber(L, 2);
        strcpy(s_log_file_dir, (const char *)lua_tostring(L, 3));
        int fd = open(s_log_file_name, O_WRONLY | O_CREAT | O_APPEND, 0600);
        if(fd == -1){
            LOG_ERROR("open file fail %s %s", s_log_file_name, strerror(errno));
            lua_pushnumber(L, 0);
            return 1;
        }else{
            if(dup2(fd, STDOUT_FILENO) == -1){
                LOG_ERROR("dup2 FAIL %s", strerror(errno));
                lua_pushnumber(L, 0);
                return 1;
            }
            close(fd);
            s_log_file = 1; 
            lua_pushnumber(L, 1);
            return 1;
        }
	}
    lua_pushnumber(L, 0);
    return 1;
}

static int Debug(lua_State *L){
	if (lua_gettop(L) == 1 && lua_isstring(L, 1)){
        size_t str_len = 0;
        const char *str = (const char *)lua_tolstring(L, 1, &str_len);
       
        log_print(DEBUG, str, str_len);
        lua_pushnumber(L, 1);
        return 1;
	}
    lua_pushnumber(L, 0);
    return 1;
}

static int Log(lua_State *L){
	if (lua_gettop(L) == 1 && lua_isstring(L, 1)){
        size_t str_len = 0;
        const char *str = (const char *)lua_tolstring(L, 1, &str_len);
       
        log_print(LOG, str, str_len);
        lua_pushnumber(L, 1);
        return 1;
	}
    lua_pushnumber(L, 0);
    return 1;
}
static int Error(lua_State *L){
	if (lua_gettop(L) == 1 && lua_isstring(L, 1)){
        size_t str_len = 0;
        const char *str = (const char *)lua_tolstring(L, 1, &str_len);

        log_print(ERROR, str, str_len);
        lua_pushnumber(L, 1);
        return 1;
	}
    lua_pushnumber(L, 0);
    return 1;
}
static int Warning(lua_State *L){
	if (lua_gettop(L) == 1 && lua_isstring(L, 1)){
        size_t str_len = 0;
        const char *str = (const char *)lua_tolstring(L, 1, &str_len);

        log_print(WARNING, str, str_len);
        lua_pushnumber(L, 1);
        return 1;
	}
    lua_pushnumber(L, 0);
    return 1;
}
static luaL_Reg lua_lib[] ={
    {"CloseAll", CloseAll},
    {"CloseLevel", CloseLevel},
    {"OpenLevel", OpenLevel},
    {"LogFile", LogFile},
    {"LogStat", LogStat},

    {"Log", Log},
    {"Error", Error},
    {"Debug", Debug},
    {"Warning", Warning},

    {NULL, NULL}
};

int log_init(lua_State *L){
	luaL_register(L, "Log", (luaL_Reg*)lua_lib);

    lua_pushstring(L, "LOG");
    lua_pushinteger(L, LOG);
    lua_settable(L, -3);

    lua_pushstring(L, "ERROR");
    lua_pushinteger(L, ERROR);
    lua_settable(L, -3);

    lua_pushstring(L, "MSG");
    lua_pushinteger(L, MSG);
    lua_settable(L, -3);

    lua_pushstring(L, "WARNING");
    lua_pushinteger(L, WARNING);
    lua_settable(L, -3);

    lua_pushstring(L, "DEBUG");
    lua_pushinteger(L, DEBUG);
    lua_settable(L, -3);

    lua_pushstring(L, "STAT");
    lua_pushinteger(L, STAT);
    lua_settable(L, -3);
    return 1;
}
