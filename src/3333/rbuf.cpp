#include "rbuf.h"
#include "fdset.h"
typedef struct RBuf{
	int buf_len;
    
    int rptr;
    int wptr;
    int used;
	char *buf;
}RBuf;


#define MAX_RBUFS 2048

static RBuf rbufs[MAX_RBUFS];

static fdset_t fdset;

static int mem_total = 0;
#define fd2rbuf(fd)  (&rbufs[fd])

#define ASSERT_FD(fd) if(fd <= 0 || fd >= MAX_RBUFS){\
                        LOG_ERROR("fd is invalid");\
                        return 0;\
                     }

int rbuf_free(int buffd){
    ASSERT_FD(buffd);
    RBuf *rbuf = fd2rbuf(buffd);
    if(rbuf->used == 0)return 0;
    rbuf->used = 1;
    fdset_push(&fdset, buffd);
    return 1;
}
void rbuf_stat(){
    LOG_STAT("==========RECV (%d)==========", mem_total/1024);
}


int rbuf_new(int size){
    int fd = fdset_pop(&fdset);
    if(fd == 0)return 0;
    RBuf *buf = &(rbufs[fd]);
    if(buf->buf_len < size){
        if(buf->buf != NULL){
            free(buf->buf);
            mem_total -= buf->buf_len;
        }
        buf->buf_len = size;
        buf->buf = (char *)malloc(size);
        mem_total += size;
        if(buf->buf == NULL){
            LOG_ERROR("malloc fail");
            return 0;
        }
    }
    buf->rptr = 0;
    buf->wptr = 0;
    buf->used = 1;
    return fd;
}

int rbuf_look_int(int buffd){
    ASSERT_FD(buffd);
    RBuf *rbuf = fd2rbuf(buffd);
    return *(int *)(rbuf->buf + rbuf->rptr);
}

int rbuf_remain(int buffd){
    ASSERT_FD(buffd);
    RBuf *rbuf = fd2rbuf(buffd);
    return rbuf->buf_len - rbuf->wptr;
}

int rbuf_len(int buffd){
    ASSERT_FD(buffd);
    RBuf *rbuf = fd2rbuf(buffd);
    return rbuf->buf_len;
}
int rbuf_size(int buffd){
    ASSERT_FD(buffd);
    RBuf *rbuf = fd2rbuf(buffd);
    return rbuf->wptr - rbuf->rptr;
}
char* rbuf_rptr(int buffd){
    ASSERT_FD(buffd);
    RBuf *rbuf = fd2rbuf(buffd);
    return rbuf->buf + rbuf->rptr;
}
char* rbuf_wptr(int buffd){
    ASSERT_FD(buffd);
    RBuf *rbuf = fd2rbuf(buffd);
    return rbuf->buf + rbuf->wptr;
}

int rbuf_rskip(int buffd, int len){
    ASSERT_FD(buffd);
    RBuf *rbuf = fd2rbuf(buffd);
    rbuf->rptr += len;
    return len;
}
int rbuf_wskip(int buffd, int len){
    ASSERT_FD(buffd);
    RBuf *rbuf = fd2rbuf(buffd);
    rbuf->wptr += len;
    return len;
}

 
int rbuf_2line(int buffd){

    ASSERT_FD(buffd);
    RBuf *rbuf = fd2rbuf(buffd);

    int plen = rbuf_look_int(buffd) + 4;

    if(rbuf->rptr == 0)return 0;
    int len = rbuf->wptr - rbuf->rptr;
    memmove(rbuf->buf, rbuf->buf + rbuf->rptr, len);
    rbuf->rptr = 0;
    rbuf->wptr = len;

    plen = rbuf_look_int(buffd) + 4;
    return 1;
}

int rbuf_init(){
    fdset_init(&fdset, MAX_RBUFS);
    return 1;
}

