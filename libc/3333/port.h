#ifndef _PORT_H_
#define _PORT_H_

#include "os.h"


typedef int (*port_cb_accept)(int portfd, int sockfd);
typedef int (*port_cb_close)(int portfd, int linefd);
typedef int (*port_cb_packet)(int portfd, int linefd, int body);

#define LINE_STATUS_CONNECTING 1
#define LINE_STATUS_CONNECTED 2
#define LINE_STATUS_DISCONNECT 0

#define MAX_PORT 64
#define MAX_LINE 4096

typedef struct LoadLine
{
    struct sockaddr_in addr;
    int linefd;
}LoadLine;

typedef struct Port
{
    int used;
    int listenfd;
    int portfd;
    int count_connection;
    
    void *userdata;
    
    port_cb_accept cb_accept;//一个client连接上来了
    port_cb_packet cb_packet;//一个完整的包到达了
    port_cb_close cb_close;//一个连接关闭了

    char lua_cb_packet[128];
    
    char lua_cb_accept[128];

    char lua_cb_close[128];
    char lua_cb_connect[128];
    char name[64];
    //配置
    int max_receive_buf_size;

    //负载均衡,和断线重连
    LoadLine *load_lines;
    int timerid;
    int load_line_num;

    //用于统计
    int packets_total;
    int startup_time;
    int bytes_total;
    int kps;
    int pps;

    int packets_ps_total;//统计即时pps
    int bytes_ps_total;//统计即时网速
    int last_ps_time;//统计即时速度
    //是否原始包
    int is_raw;
}Port;

typedef struct Line
{
    int used;
    int status;//LINE_STATUS_XXX
    int is_reconnect;
    int portfd;
    int linefd;
    int sockfd;
    char host[32];
    int port;
    unsigned char close_flag; 
    int uid;
    //读缓冲区
    int buf_read;
    //写缓冲区
    int buf_write;
    int send_seq; 
    int recv_seq; 
}Line;

extern Line s_lines[MAX_LINE];

void port_stat();
int port_init(lua_State *L);
int port_new();
int port_set_raw(int portfd);
int port_set_cb_accept(int portfd, port_cb_accept cb);
int port_set_cb_packet(int portfd, port_cb_packet cb);
int port_set_cb_close(int portfd, port_cb_close cb);

int line_notify_writable(int linefd);
int line_is_close(int fd);

Port* port_from_fd(int portfd);
Line* line_from_fd(int linefd);

#endif
