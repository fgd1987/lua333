#ifndef _LOG_H_
#define _LOG_H_

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
extern "C"{

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

#define ERROR   0
#define WARNING 1
#define LOG     2
#define MSG     3
#define DEBUG 4
#define STAT 5

int log_init(lua_State *L);

int log_log(const char *fmt, ...);
int log_warning(const char *fmt, ...);
int log_error(const char *fmt, ...);
int log_msg(const char *fmt, ...);
int log_debug(const char *fmt, ...);
int log_stat(const char *fmt, ...);

int log_stat_();
#define LOG_ERROR(str, ...)      log_error("%s:%d", __FILE__, __LINE__);\
                                 log_error(str, ## __VA_ARGS__);

#define LOG_WARNING(str, ...)    log_warning("%s:%d", __FILE__, __LINE__);\
                                 log_warning(str, ## __VA_ARGS__);

#define LOG_LOG(str, ...)        log_log("%s:%d", __FILE__, __LINE__);\
                                 log_log(str, ## __VA_ARGS__);


#define LOG_MSG(str, ...)        log_msg(str, ## __VA_ARGS__);

#define LOG_DEBUG(str, ...)    log_debug(str, ## __VA_ARGS__);

#define LOG_STAT(str, ...)    log_stat(str, ## __VA_ARGS__);

#endif
