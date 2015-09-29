#include "port.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

static Port s_ports[MAX_PORT];
Line s_lines[MAX_LINE];

#define fd2port(fd)  (&s_ports[fd])
#define fd2line(fd)  (&s_lines[fd])

static fdset_t fdset;

Port* port_from_fd(int portfd){
    if(portfd <= 0 || portfd >= MAX_PORT){
        LOG_ERROR("bad fd %d", portfd);
        return NULL;
    }
    return fd2port(portfd);
}

Line* line_from_fd(int linefd){
    if(linefd <= 0 || linefd >= MAX_LINE){
        LOG_ERROR("bad fd %d", linefd);
        return NULL;
    }
    return fd2line(linefd);
}


static int line_new(){
    int fd = fdset_pop(&fdset);;
    if(fd == 0){ LOG_ERROR("linefd reach limit");
        return fd;
    }
    //初始化port
    Line *line = fd2line(fd);
    bzero(line, sizeof(Line));
    line->used = 1;
    line->uid = 0;
    line->send_seq = 0;
    line->recv_seq = 0;
    line->is_reconnect = 0;
    line->close_flag = 0;
    line->linefd = fd;
    return fd;
}
int line_is_close(int fd){
    if(fd <= 0 || fd >= MAX_LINE){
        LOG_ERROR("bad fd %d", fd); 
        return 1;
    }
	Line *line = fd2line(fd);
    if(line->used == 0){
        return 1;
    }
    if(line->status != LINE_STATUS_CONNECTED){
        return 1;
    }
    return 0;
}
static int line_free(int fd){
    if(fd <= 0 || fd >= MAX_LINE){
        LOG_ERROR("bad fd %d", fd); 
        return 0;
    }
    Line *line = fd2line(fd);
    if(line->used == 0){
        return 0;
    }
    fdset_push(&fdset, fd);
    line->used = 0;
    line->portfd = 0;
    line->buf_read = 0;
    line->buf_write = 0;
    return 1;
}

static int port_close_line(int linefd, const char *reason);
static void port_on_accept(struct aeEventLoop *eventLoop, int listenfd, void *args, int event);
static void setnonblock(int sockfd);
static int port_accept(int portfd, int sockfd);
static int port_free(int portfd);
static int port_stat_timer(int msgid, void *userdata);

static void port_on_accept(struct aeEventLoop *eventLoop, int listenfd, void *args, int event){
    Port* port = (Port *)args;	
    if(port == NULL){
        LOG_ERROR("null");
        return;
    }
	int sockfd;	
    int portfd;
	struct sockaddr_in addr;	
	socklen_t addrlen = sizeof(addr);	
    portfd = port->portfd;
    
    sockfd = accept(listenfd, (struct sockaddr*)&addr, &addrlen);
    if(sockfd == -1){
        LOG_ERROR("accept fail errno %d %s", errno, strerror(errno));
        return;
    }
    LOG_LOG("%s a client connected ! portfd:%d sockfd:%d handler:%x\n", port->name, portfd, sockfd, port->cb_accept);
    if(port->cb_accept)
    {
        port->cb_accept(portfd, sockfd);
        if(port_accept(portfd, sockfd)){
            LOG_ERROR("accept fail");
            close(sockfd);
            return;
        }
    }
    else if(port->lua_cb_accept[0] != 0){

        int linefd = port_accept(portfd, sockfd);
        if(linefd <= 0){
            LOG_ERROR("accept fail");
            close(sockfd);
            return;
        }
       	luas_callfunc(port->lua_cb_accept, "(%d)", linefd);
    }
    else{
        close(sockfd);
    }
}
int port_set_raw(int portfd){
    Port *port = fd2port(portfd);
    port->is_raw = 1;
    return 1;
}
static int port_dispatch(int linefd){
    if(linefd <= 0 || linefd >= MAX_LINE){
        LOG_ERROR("bad fd %d", linefd); 
        return 0;
    }
    Line *line = fd2line(linefd);
    int buffd = line->buf_read;
    int portfd = line->portfd;
    Port *port = fd2port(portfd);

    if(port->is_raw == 1){
        int str_len = rbuf_size(buffd);
        char *str = rbuf_rptr(buffd);
        if(port->lua_cb_packet[0] != 0){
            luas_pushluafunction(port->lua_cb_packet);
            lua_pushnumber(L, linefd);
            lua_pushlstring(L, str, str_len);
            if (lua_pcall(L, 2, 1, 0) != 0){
                LOG_ERROR("error running function %s: %s", port->lua_cb_packet, lua_tostring(L, -1));
            }
            lua_pop(L, lua_gettop(L));
        }
        //没读完的跳过
        rbuf_rskip(buffd, str_len);
        return 1;
    }

    for(;;){
        int buf_size = rbuf_size(buffd);
        if(buf_size <= 0){
            break;
        }
        int plen = rbuf_look_int(buffd) + 4;
        //LOG_LOG("plen %d buf_size %d", plen, buf_size);
        if(plen <= 4){
            break;
        }
        if(plen > rbuf_len(buffd)){
            //这个包超过了最大的限制
            LOG_ERROR("port %s receive a packet reach limit %d/%d", port->name, plen, rbuf_len(buffd));
            //close it?
            return 0;
        }
        if(buf_size < plen){
            //LOG_LOG("not enouth %d/%d", buf_size, plen);
            break;
        }
        char *rptr = rbuf_rptr(buffd);
        if(port->cb_packet){

            int arfd = ar_new(rptr, plen);
            port->cb_packet(portfd, linefd, arfd);
            ar_free(arfd);
        }else if(port->lua_cb_packet[0] != 0){

            int arfd = ar_new(rptr, plen);
        	luas_callfunc(port->lua_cb_packet, "(%d, %d)",
        			linefd, arfd);
            ar_free(arfd);
        }
        //没读完的跳过
        rbuf_rskip(buffd, plen);

        port->packets_ps_total++;
        port->packets_total++;
    }
    return 1;
}
static void port_on_read(struct aeEventLoop *eventLoop, int sockfd, void *args, int event)
{
    Line *line = (Line *)args;
    int linefd = line->linefd;
    int portfd = line->portfd;
    int buffd = line->buf_read;
    Port *port = fd2port(portfd);

    int   wlen = rbuf_remain(buffd);
    char* wbuf = rbuf_wptr(buffd);
    int rt = recv(sockfd, wbuf, wlen, 0);
//    LOG_LOG("[%d]read str %d addr:%x buffd %d from %s(%d)", line->uid, rt, wbuf, buffd, port->name, sockfd);

    if(rt > 0){
    	port->bytes_ps_total+=rt;
    	port->bytes_total+=rt;
    }
    if(rt > 0){       
        //读完了
        rbuf_wskip(buffd, rt);
        goto dispatch;
    }else if(rt < 0 && errno == EAGAIN){
        //读完了
        goto dispatch;
    }else{
        //LOG_LOG("rt %d errno %d errstr %s", rt, errno, strerror(errno));
        goto close;;
    }
dispatch:
    if(port_dispatch(linefd) == 0){
        //立刻关闭
        shutdown(sockfd, SHUT_RDWR);
        goto close;
    }
    rbuf_2line(buffd);

    return;
close:
    port_close_line(linefd, strerror(errno));
    return;
}
static void port_on_write(struct aeEventLoop *eventLoop, int sockfd, void *args, int event){
    //LOG_LOG("on_write");
    Line *line = (Line *)args;
    int linefd = line->linefd;
    int buffd = line->buf_write;
    while(true){
        int   rlen = bbuf_rlen(buffd);
        char* rbuf = bbuf_rptr(buffd);
        if(rlen == 0){
            break;
        }else if(rlen > 0){
            int ir = send(sockfd, rbuf, rlen, 0);
            //LOG_LOG("write str %d to %s(%d)", ir, port->name, sockfd);
            if(ir > 0){
                bbuf_rskip(buffd, ir);
            }else if(ir == 0){
                //写满了?
                break;
            }else if(ir == -1 && errno == EAGAIN){
                //写满了?
                break;
            }else if(ir == -1){
            //LOG_LOG("port_on_write")
                port_close_line(linefd, "peer close");
                break;
            }
        }
    }
    if(bbuf_rlen(buffd) <= 0){
        //当前没有数据可写了
        aeDeleteFileEvent(evt_loop(), sockfd, AE_WRITABLE);
        //是否要关闭了
        if(line->close_flag == 1){
            //line->close_flag = 2;
            //关闭读/写端
            //shutdown(sockfd, SHUT_WR);
            //close(sockfd);
            port_close_line(linefd, strerror(errno));
        }
    }
}

static int port_close_line(int linefd, const char *reason){
    if(linefd <= 0 || linefd >= MAX_LINE){
        LOG_ERROR("bad fd %d", linefd); 
        return 0;
    }
    Line *line = fd2line(linefd);
    int portfd = line->portfd;
    //LOG_LOG("port_close_line")
    if(portfd <= 0 || portfd >= MAX_PORT){
        LOG_ERROR("bad fd %d", portfd); 
        return 0;
    }
    Port *port = fd2port(portfd);
    if(line->used == 0){
        LOG_ERROR("is not used");
        return 0;
    }
    if(port->cb_close)
    {
    	port->cb_close(portfd, linefd);
    }else if(port->lua_cb_close[0] != 0){

        lua_State *L = luas_open();
        luas_pushluafunction(port->lua_cb_close);
        //sock 
        lua_pushnumber(L, linefd);
        //reason
        lua_pushstring(L, reason);

        if (lua_pcall(L, 2, 1, 0) != 0){
            LOG_ERROR("error running function %s: %s", port->lua_cb_close, lua_tostring(L, -1));
        }
        lua_pop(L, lua_gettop(L));
    }
    line->status = LINE_STATUS_DISCONNECT;
    int sockfd = line->sockfd;

    //移动事件
    aeDeleteFileEvent(evt_loop(), sockfd, AE_READABLE);
    aeDeleteFileEvent(evt_loop(), sockfd, AE_WRITABLE);
    //释放buff

    if(line->buf_read){
        rbuf_free(line->buf_read);
    }
    if(line->buf_write){
        bbuf_free(line->buf_write);
    }
    line->buf_read = 0;
    line->buf_write = 0;
    //关闭socket
    close(sockfd);
    if(line->is_reconnect == 0){
        //释放line
        line_free(linefd);
    }else{
        //由port自己负责释放
    }
    return 1;
}

static void setnonblock(int sockfd){
    int flags;
    flags = fcntl(sockfd, F_GETFL);
    flags |= O_NONBLOCK;
    fcntl(sockfd, F_SETFL, flags);
}

static int port_accept(int portfd, int sockfd){
    if(portfd <= 0 || portfd >= MAX_PORT){
        LOG_ERROR("bad fd %d", portfd);
        return 0;
    }
    int linefd = line_new();
    if(linefd <= 0 || linefd >= MAX_LINE){   
        LOG_ERROR("port_accept line is full portfd:%d, sockfd:%d", portfd, sockfd);
        close(sockfd); 
        return 0;
    }
    Port *port = fd2port(portfd);
    Line *line = fd2line(linefd);
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    getpeername(sockfd, (struct sockaddr *)&addr, &addr_len);
    strcpy(line->host, inet_ntoa(addr.sin_addr));
    line->port = ntohs(addr.sin_port);
    setnonblock(sockfd);
    //初始化
    line->status = LINE_STATUS_CONNECTED;
    line->sockfd = sockfd;
    line->portfd = portfd;
    line->buf_read = rbuf_new(port->max_receive_buf_size);
    if(line->buf_read == 0){
        LOG_ERROR("buf read alloc fail");
        line_free(linefd);
        return 0;
    }
    line->buf_write = bbuf_new(1024);
    if(line->buf_write == 0){
        bbuf_free(line->buf_read);
        LOG_ERROR("buf write alloc fail");
        line_free(linefd);
        return 0;
    }
    aeCreateFileEvent(evt_loop(), sockfd, AE_READABLE, port_on_read, (void*)line);
    aeCreateFileEvent(evt_loop(), sockfd, AE_WRITABLE, port_on_write, (void*)line);

    return linefd;
}

int port_new(){
    int fd = 0;
    int i;
    for(i = 1; i < MAX_PORT; i++){
        if(s_ports[i].used == 0){   
            int timenow = time(NULL);
            fd = i;
            //初始化port
            bzero(&s_ports[i], sizeof(Port));
            s_ports[i].used = 1;
            s_ports[i].portfd = fd;
            s_ports[i].load_lines = NULL;
            //stat
            s_ports[i].packets_total = 0;
            s_ports[i].startup_time = timenow;
            s_ports[i].bytes_total = 0;
            s_ports[i].packets_ps_total = 0;
            s_ports[i].bytes_ps_total = 0;
            s_ports[i].last_ps_time = timenow;
            s_ports[i].lua_cb_packet[0] = 0;
            s_ports[i].lua_cb_accept[0] = 0;
            s_ports[i].max_receive_buf_size = 2048;
            s_ports[i].is_raw = 0;
            break;
        }
    }
    return fd;
}
static int port_free(int portfd){
	Port *port = fd2port(portfd);
	port->used = 0;
    if(port->load_lines != NULL){
        free(port->load_lines);
        port->load_lines = NULL;
    }
	return 1;
}
void  port_set_userdata(int portfd, void *userdata){
    Port *port = fd2port(portfd);
    port->userdata = userdata;
}
void *port_get_userdata(int portfd){
    Port *port = fd2port(portfd);
    return port->userdata;
}
int port_set_cb_accept(int portfd, port_cb_accept cb){
    Port *port = fd2port(portfd);
    port->cb_accept = cb;
    return 1;
}

int port_set_cb_close(int portfd, port_cb_close cb){
    Port *port = fd2port(portfd);
    port->cb_close = cb;
    return 1;
}
int port_set_cb_packet(int portfd, port_cb_packet cb){
    Port *port = fd2port(portfd);
    port->cb_packet = cb;
    return 1;
}




static void port_on_connect_success(struct aeEventLoop *eventLoop, int sockfd, void *args, int event){
    Line *line = (Line *)args;
    int linefd = line->linefd;
    int portfd = line->portfd;
    Port *port = fd2port(portfd);

    int error;
    socklen_t len;
    if(true || getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len) != 0){

        LOG_ERROR("connect success, but sockfd %d, error %d, strerror:%s", sockfd, error,  strerror(errno));

    }else if(error == 128){
        LOG_LOG("operation now in progress");
        return;
    }
    else if(error != 0){
        LOG_ERROR("connect success, but sockfd %d, error %d, strerror:%s", sockfd, error,  strerror(errno));

        line->status = LINE_STATUS_DISCONNECT;
        port_close_line(linefd, strerror(errno));
        return;
    }
    aeDeleteFileEvent(evt_loop(), sockfd, AE_READABLE);
    aeDeleteFileEvent(evt_loop(), sockfd, AE_WRITABLE);

    line->buf_read = rbuf_new(port->max_receive_buf_size);
    if(line->buf_read == 0){
        LOG_ERROR("alloc buf read fail");
        line->status = LINE_STATUS_DISCONNECT;
        port_close_line(linefd, strerror(errno));
        return;
    }
    line->buf_write = bbuf_new(1024);
    if(line->buf_write == 0){
        LOG_ERROR("alloc buf write fail");

        rbuf_free(line->buf_read);
        line->status = LINE_STATUS_DISCONNECT;
        port_close_line(linefd, strerror(errno));
        return;
    }

    aeCreateFileEvent(evt_loop(), sockfd, AE_READABLE, port_on_read, (void*)line);
    aeCreateFileEvent(evt_loop(), sockfd, AE_WRITABLE, port_on_write, (void*)line);

    line->status = LINE_STATUS_CONNECTED;
    if(port->lua_cb_connect[0] != 0){
        lua_State *L = luas_open();
        luas_pushluafunction(port->lua_cb_connect);
        //sock 
        lua_pushnumber(L, linefd);
        //host
        lua_pushstring(L, line->host);
        //port
        lua_pushinteger(L, line->port);
        if (lua_pcall(L, 3, 0, 0) != 0){
            LOG_ERROR("error running function %s: %s", port->lua_cb_connect, lua_tostring(L, -1));
        }
        lua_pop(L, lua_gettop(L));
    }

}

static void port_on_connect_error(struct aeEventLoop *eventLoop, int sockfd, void *args, int event){

    Line *line = (Line *)args;
    int linefd = line->linefd;
    line->status = LINE_STATUS_DISCONNECT;
    Port *port = fd2port(line->portfd);
    LOG_WARNING("[%s] conect fail %s", port->name, line->host);
    port_close_line(linefd, strerror(errno));
}
int line_sync_connect_addr(int portfd, struct sockaddr_in addr){
    if(portfd <=0 || portfd >= MAX_PORT){
        LOG_ERROR("bad fd %d", portfd);
        return 0;
    }
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0){
        LOG_WARNING("port_connect create socket fail");
        return 0;
    }
    int rt = connect(sockfd, (struct sockaddr *)&addr, sizeof(addr));
    if(rt < 0){
        close(sockfd);
        LOG_ERROR("port_connect fail errno:%d:%s", errno, strerror(errno));
        return 0;
    }
    int linefd = line_new();
    if(linefd <= 0){
        LOG_ERROR("line fd reach limit");
        close(sockfd);
        return 0;
    }
    setnonblock(sockfd);
    Line *line = fd2line(linefd);
    Port *port = fd2port(portfd);
    line->portfd = portfd;
    line->is_reconnect = 0;
    line->status = LINE_STATUS_CONNECTED;
    line->sockfd = sockfd;
    line->portfd = portfd;
    line->buf_read = rbuf_new(port->max_receive_buf_size);
    if(line->buf_read == 0){
        LOG_ERROR("buf read alloc fail");
        line_free(linefd);
        return 0;
    }
    line->buf_write = bbuf_new(1024);
    if(line->buf_write == 0){
        bbuf_free(line->buf_read);
        LOG_ERROR("buf write alloc fail");
        line_free(linefd);
        return 0;
    }
   
    aeCreateFileEvent(evt_loop(), sockfd, AE_READABLE, port_on_read, (void*)line);
    aeCreateFileEvent(evt_loop(), sockfd, AE_WRITABLE, port_on_write, (void*)line);

    return linefd;
}

int line_connect_addr(int linefd, struct sockaddr_in addr)
{
    if(linefd <= 0 || linefd >= MAX_LINE){
        return 0;
    }
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0){
        LOG_ERROR("create socket fail\n");
        return 0;
    }

    Line *line = fd2line(linefd);
    int portfd = line->portfd;
    line->status = LINE_STATUS_CONNECTING;
    setnonblock(sockfd);
    //初始化
    line->sockfd = sockfd;
    line->portfd = portfd;
   

    aeCreateFileEvent(evt_loop(), sockfd, AE_READABLE, port_on_connect_error, (void*)line);
    aeCreateFileEvent(evt_loop(), sockfd, AE_WRITABLE, port_on_connect_success, (void*)line);


    int rt = connect(sockfd, (struct sockaddr *)&addr, sizeof(addr));
    if(rt < 0){
        //LOG_ERROR("connect fail errno:%d: %s", errno, strerror(errno));
        return 0;
    }
    return 1;
}
int port_listen(int portfd, unsigned short portno){
    int listenfd;
	struct sockaddr_in addr;
    if(portfd <= 0 || portfd >= MAX_PORT){
        LOG_ERROR("bad fd %d", portfd);
        return 0;
    }
    Port *port;
    port = fd2port(portfd);
	listenfd = socket(AF_INET, SOCK_STREAM, 0);	
	if(listenfd < 0){		
		LOG_ERROR("create socket fail %d %s", errno, strerror(errno));
		port_free(portfd);
		return 0;	
	}	
	bzero((void*)&addr, sizeof(addr));	
	addr.sin_family = AF_INET;	
	addr.sin_addr.s_addr = INADDR_ANY;	
	addr.sin_port = htons(portno);	
	
    int reuse = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    
	if(bind(listenfd,(struct sockaddr *)&addr,sizeof(addr)) < 0){		
        LOG_ERROR("fail to bind %d %d %s", portno, errno, strerror(errno));
		port_free(portfd);
		return 0;	
	}	
	if(listen(listenfd, 5) < 0){		
        LOG_ERROR("fail to listen %d %s", errno, strerror(errno));
		port_free(portfd);
		return 0;	
	}
    port->listenfd = listenfd;
    aeCreateFileEvent(evt_loop(), listenfd, AE_READABLE, port_on_accept, (void*)port);
    return 1;
}

void port_stat(){
    LOG_STAT("==========PORT==========")
	for(int i = 1; i < MAX_PORT; i++){
		if(s_ports[i].used == 0)continue;
		Port *port = &s_ports[i];
        LOG_STAT("%s total packet:%d total bytes:%d pps:%d kps:%d", port->name, 
            port->packets_total, port->bytes_total, port->pps, port->kps);
    }
    LOG_STAT("==========LINE(%d/%d)==========", fdset_top(&fdset), fdset_size(&fdset))
}

static int port_stat_timer(int msgid, void *userdata){
	int i;
	for(i = 1; i < MAX_PORT; i++){
		if(s_ports[i].used == 0)continue;
		Port *port = &s_ports[i];

		int timenow = time(NULL);
		int ds = timenow - port->last_ps_time;
		ds = max(ds, 1);
		port->pps = (port->packets_ps_total)/ds;
		port->kps = (port->bytes_ps_total/1000)/ds;

		port->last_ps_time = time(NULL);
		port->bytes_ps_total = 0;
		port->packets_ps_total = 0;
	}
	return 1;
}
int line_notify_writable(int linefd){
    if(linefd <= 0 || linefd >= MAX_LINE){
        LOG_ERROR("bad fd %d", linefd);
        return 0;
    }
	Line *line = fd2line(linefd);
    aeCreateFileEvent(evt_loop(), line->sockfd, AE_WRITABLE, port_on_write, (void*)line);
    return 1;
}

static int Listen(lua_State *L)
{
    if(lua_isnumber(L, 1) && lua_isnumber(L, 2)){
        int portfd = (int)lua_tonumber(L, 1);
        if(portfd <= 0 || portfd >= MAX_PORT){
            lua_pushnil(L);
            return 1;
        }
        unsigned short portno = (unsigned short)lua_tonumber(L, 2);
        if(port_listen(portfd, portno) == 0){
            lua_pushnil(L);
            return 1;
        }
        lua_pushboolean(L, true);
        return 1;
    }
    lua_pushnil(L);
    return 1;
}
static int SetReceiveBuf(lua_State *L){
    if(lua_isnumber(L, 1) && lua_isnumber(L, 2)){
        int portfd = (int)lua_tonumber(L, 1);
        if(portfd <= 0 || portfd >= MAX_PORT){
            lua_pushnil(L);
            return 1;
        }
        int size = (int)lua_tonumber(L, 2);
        Port *port = fd2port(portfd);
        port->max_receive_buf_size = size;
        lua_pushboolean(L, true);
        return 1;
    }
    lua_pushnil(L);
    return 1;
}

static int GetPeerIpAddr(lua_State *L)
{
    if (lua_isnumber(L, 1))
    {
        int linefd = (int)lua_tonumber(L, 1);
        if(linefd <= 0 || linefd >= MAX_LINE){
            lua_pushnil(L);
            return 1;
        }
	    Line *line = fd2line(linefd);
        lua_pushnumber(L, inet_addr(line->host));
        return 1;
    }
    lua_pushnil(L);
    return 1;
}

static int GetPeerHost(lua_State *L)
{
    if (lua_isnumber(L, 1))
    {
        int linefd = (int)lua_tonumber(L, 1);
        if(linefd <= 0 || linefd >= MAX_LINE){
            lua_pushnil(L);
            return 1;
        }
	    Line *line = fd2line(linefd);
        lua_pushstring(L, line->host);
        return 1;
    }
    lua_pushnil(L);
    return 1;
}
static int GetPeerPort(lua_State *L)
{
    if (lua_isnumber(L, 1))
    {
        int linefd = (int)lua_tonumber(L, 1);
        if(linefd <= 0 || linefd >= MAX_LINE){
            lua_pushnil(L);
            return 1;
        }
	    Line *line = fd2line(linefd);
        lua_pushinteger(L, line->port);
        return 1;
    }
    lua_pushnil(L);
    return 1;
}


/*
 *尝试重连
 */
static int port_load_line_status(int msgid, void *userdata){
    Port *port = (Port *)userdata;
    if(port == NULL){
        return 0;
    }
    for(int i = 0; i < port->load_line_num; i++){
        LoadLine *loadline = &(port->load_lines[i]);
        if(loadline->linefd == 0){
        }else{
            Line *line = fd2line(loadline->linefd);
            if(line->status == LINE_STATUS_DISCONNECT){
                LOG_WARNING("[%s] line %d sockfd:%d is disconnect, reconnect",  port->name, loadline->linefd, line->sockfd);
                line_connect_addr(loadline->linefd, loadline->addr);
            }
        }
    }
    return 1;
}

static int Connect(lua_State *L){   
    int portfd = (int)LUA_TONUMBER(1);
    if(portfd <= 0){
        LOG_ERROR("port_connect invalid portfd:%d", portfd);
        return 0;
    }
    const char *ip = (const char *)LUA_TOSTRING(2);
    unsigned short portno = (unsigned short)LUA_TONUMBER(3);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
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
    addr.sin_port = htons(portno);        
    int linefd = line_sync_connect_addr(portfd, addr);
    lua_pushnumber(L, linefd);
    return 1;
}


static int Select(lua_State *L){
    int portfd = (int)LUA_TONUMBER(1);
    if(portfd <= 0){
        lua_pushnil(L);
        return 1;
    }
    int base = (int)lua_tonumber(L, 2);
    Port *port = fd2port(portfd);
    int idx = base % port->load_line_num;
    int linefd = port->load_lines[idx].linefd;
    lua_pushinteger(L, linefd);
    return 1;
}
//关闭由ConnectV打开的socket和port
static int CloseClient(lua_State *L){
    int portfd = (int)LUA_TONUMBER(1);
    if(portfd <= 0){
        return 0;
    }
    Port *port = fd2port(portfd);
    if(port == NULL){
        return 0;
    }
    for(int i = 0; i < port->load_line_num; i++){
        LoadLine *loadline = &(port->load_lines[i]);
        if(loadline->linefd == 0){
        }else{
            Line *line = fd2line(loadline->linefd);
            if(line->status == LINE_STATUS_CONNECTED){
                line->is_reconnect = 0;
                port_close_line(line->linefd, "");
            }
        }
    }
    port_free(portfd);
    timer_stop(port->timerid);
    return 1;
}
static int ConnectAsync(lua_State *L){   
    int portfd = (int)LUA_TONUMBER(1);
    if(portfd <= 0){
        LOG_ERROR("port_connect invalid portfd:%d", portfd);
        return 0;
    }
    const char *ip = (const char *)LUA_TOSTRING(2);
    unsigned short portno = (unsigned short)LUA_TONUMBER(3);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
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
    addr.sin_port = htons(portno);        

    int linefd = line_new();
    if(linefd <= 0){   
        LOG_ERROR("linefd reach limit");
        lua_pushnil(L);
        return 1;
    }
    Line *line = fd2line(linefd);
    line->portfd = portfd;
    line->status = LINE_STATUS_DISCONNECT;
    line->is_reconnect = 1;//是一个连接到对方的链接 
    strcpy(line->host, ip);
    line->port = portno;

    line_connect_addr(linefd, addr);
    lua_pushboolean(L, true);
    return 1;
} 
static int ConnectV(lua_State *L)
{   
    int i ;
    int argnum = lua_gettop(L);
    int portfd = (int)lua_tonumber(L, 1);
    if(portfd <= 0 || portfd >= MAX_PORT){
        LOG_ERROR("bad fd %d", portfd);
        lua_pushnil(L);
        return 1;
    }
    Port *port = fd2port(portfd);
    int linenum = (argnum-1) / 2;
    int mem_size = sizeof(LoadLine) * linenum;
    port->load_lines = (LoadLine *)malloc(mem_size);
    if(port->load_lines == NULL){
        LOG_ERROR("malloc fail fd %d", portfd);
        lua_pushnil(L);
        return 1;
    }
    port->load_line_num = linenum;
    for(i = 0; i < linenum; i++)
    {
        LoadLine *loadline = &(port->load_lines[i]);
        const char *ip = (const char *)lua_tostring(L, i * 2 + 2);
        unsigned short portno = (unsigned short)lua_tonumber(L,  i * 2 + 3);
        loadline->addr.sin_family = AF_INET;

        if(inet_addr(ip) != (in_addr_t)-1){
           loadline->addr.sin_addr.s_addr = inet_addr(ip);   
        }else{
            struct hostent *hostent;
            hostent = gethostbyname(ip);
            if(hostent->h_addr_list[0] == NULL){
                LOG_ERROR("connect fail %s", ip);
                return 0;
            }
            //LOG_LOG("ip %s/n", inet_ntoa(*(struct in_addr*)(hostent->h_addr_list[0])));
            memcpy(&loadline->addr.sin_addr, (struct in_addr *)hostent->h_addr_list[0], sizeof(struct in_addr));
        } 

        loadline->addr.sin_port = htons(portno);        
        int linefd = line_new();
        if(linefd <= 0)
        {   
            LOG_ERROR("linefd reach limit");
            lua_pushnil(L);
            return 1;
        }
        Line *line = fd2line(linefd);
        line->portfd = portfd;
        line->status = LINE_STATUS_DISCONNECT;
        line->is_reconnect = 1;//是一个连接到对方的链接 
        strcpy(line->host, ip);
        line->port = portno;
        loadline->linefd = linefd;
    }
    //首次尝试连接
    for(i = 0; i < linenum; i++){
        LoadLine *loadline = &(port->load_lines[i]);
        int linefd = loadline->linefd;
        if(linefd <= 0){
            continue;
        }
        line_connect_addr(linefd, loadline->addr);
    }
    //加入定时器, 维护链接状态
    port->timerid = timer_add_callback(port_load_line_status, 1000, (void*)port);
    lua_pushboolean(L, true);
    return 1;
}

static int OnPacket(lua_State *L)
{
    if (lua_gettop(L) == 2 && lua_isnumber(L, 1) && lua_isstring(L, 2))
    {
        int portfd 					= (int)lua_tonumber(L, 1);
        const char *funcname 	= (const char *)lua_tostring(L, 2);
        if(portfd <= 0 || portfd >= MAX_PORT){
            LOG_ERROR("bad fd %d", portfd);
            lua_pushnil(L);
            return 1;
        }
	    Port *port = fd2port(portfd);
	    strcpy(port->lua_cb_packet, funcname);
        lua_pushboolean(L, true);
        return 1;
    }
    lua_pushnil(L);
    return 1;
}

static int onconnect(lua_State *L)
{
    if (lua_gettop(L) == 2 && lua_isnumber(L, 1) && lua_isstring(L, 2))
    {
        int portfd = (int)lua_tonumber(L, 1);
        const char *funcname = (const char *)lua_tostring(L, 2);
        if(portfd <= 0 || portfd >= MAX_PORT){
            LOG_ERROR("bad fd %d", portfd);
            return 0;
        }
	    Port *port = fd2port(portfd);
	    strcpy(port->lua_cb_connect, funcname);
        lua_pushboolean(L, 1);
        return 1;
    }
    return 0;
}

static int rename(lua_State *L)
{
    if (lua_gettop(L) == 2 && lua_isnumber(L, 1) && lua_isstring(L, 2))
    {
        int portfd = (int)lua_tonumber(L, 1);
        const char *name = (const char *)lua_tostring(L, 2);
        if(portfd <= 0 || portfd >= MAX_PORT){
            LOG_ERROR("bad fd %d", portfd);
            lua_pushnil(L);
            return 1;
        }
	    Port *port = fd2port(portfd);
	    strcpy(port->name, name);
        lua_pushboolean(L, true);
        return 1;
    }
    lua_pushnil(L);
    return 1;
}

static int onaccept(lua_State *L)
{
    if (lua_gettop(L) == 2 && lua_isnumber(L, 1) && lua_isstring(L, 2))
    {
        int portfd = (int)lua_tonumber(L, 1);
        const char *funcname = (const char *)lua_tostring(L, 2);
        if(portfd <= 0 || portfd >= MAX_PORT){
            return 0;
        }
	    Port *port = fd2port(portfd);
	    strcpy(port->lua_cb_accept, funcname);
        lua_pushboolean(L, true);
        return 1;
    }
    lua_pushnil(L);
    return 1;
}

static int setuid(lua_State *L){
    if(lua_isnumber(L, 1) && lua_isnumber(L, 2)){
        int portfd = (int)lua_tonumber(L, 1);
        int linefd = (int)lua_tonumber(L, 2);
        int uid = (int)lua_tonumber(L, 3);
        if(portfd <= 0 || portfd >= MAX_PORT){
            LOG_ERROR("bad fd %d", portfd);
            lua_pushnil(L);
            return 1;
        }
        if(linefd <= 0 || linefd >= MAX_LINE){
            LOG_ERROR("bad fd %d", linefd);
            lua_pushnil(L);
            return 1;
        }
        Line *line = fd2line(linefd);
        line->uid = uid;
        lua_pushboolean(L, true);
        return 1;
    }
    lua_pushnil(L);
    return 1;
}

static int syncclose(lua_State *L){
    //LOG_LOG("SyncClose");
    if(lua_isnumber(L, 1) && lua_isnumber(L, 2))
    {
        int portfd = (int)lua_tonumber(L, 1);
        int linefd = (int)lua_tonumber(L, 2);
        if(portfd <= 0 || portfd >= MAX_PORT){
            LOG_ERROR("bad fd %d", portfd);
            return 0;
        }
        if(linefd <= 0 || linefd >= MAX_LINE){
            LOG_ERROR("bad fd %d", linefd);
            return 0;
        }
        port_close_line(linefd, "");
        lua_pushboolean(L, 1);
        return 1;
    }
    return 0;
}


static int close(lua_State *L){
    if(lua_isnumber(L, 1) && lua_isnumber(L, 2)){
        int portfd 	= (int)lua_tonumber(L, 1);
        int linefd 	= (int)lua_tonumber(L, 2);
        if(portfd <= 0 || portfd >= MAX_PORT){
            LOG_ERROR("bad fd %d", portfd);
            return 0;
        }
        if(linefd <= 0 || linefd >= MAX_LINE){
            LOG_ERROR("bad fd %d", linefd);
            return 0;
        }
        Line *line = fd2line(linefd);
        int sockfd = line->sockfd;
        //关闭读
        aeDeleteFileEvent(evt_loop(), sockfd, AE_READABLE);
        shutdown(sockfd, SHUT_RD);
        line->close_flag = 1;
        line_notify_writable(linefd);
        lua_pushboolean(L, true);
        return 1;
    }
    lua_pushnil(L);
    return 1;
}


static int OnClose(lua_State *L){
    if (lua_gettop(L) == 2 && lua_isnumber(L, 1) && lua_isstring(L, 2))
    {
        int portfd 					= (int)lua_tonumber(L, 1);
        const char *funcname 	= (const char *)lua_tostring(L, 2);
        if(portfd <= 0 || portfd >= MAX_PORT){
            LOG_ERROR("bad fd %d", portfd);
            lua_pushnil(L);
            return 1;
        }

        Port *port = fd2port(portfd);
	    strcpy(port->lua_cb_close, funcname);
        lua_pushboolean(L, true);
        return 1;
    }
    lua_pushnil(L);
    return 1;
}


static int lcreate(lua_State *L){
    if (lua_gettop(L) == 0)
    {
        int portfd = port_new();
        lua_pushinteger(L, portfd);
        return 1;
    }
    lua_pushnil(L);
    return 1;
}

static luaL_Reg lua_lib[] = 
{
    {"listen", listen},
    {"donnectv", ConnectV},
    {"connectasync", ConnectAsync},
    {"select", select},
    {"connect", connect},
    {"onpacket", onpacket},
    {"onaccept", Onaccept},
    {"rename", rename},
    {"onconnect", OnConnect},
    {"onclose", OnClose},
    {"close", CloseLine},
    {"SyncClose", SyncClose},
    {"getpeerport", getpeerport},
    {"getpeerhost", getpeerhost},
    {"getPeeripaddr", getpeeripaddr},
    {"CloseClient", CloseClient},
    {"create", lcreate},
    {"SetUid", SetUid},
    {NULL, NULL},
};

int port_init(lua_State *L){
	luaL_register(L, "Port", (luaL_Reg*)lua_lib);
    return 1;
}
