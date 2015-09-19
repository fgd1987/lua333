
typedef struct RBuf{
	int buf_len;
    int rptr;
    int wptr;
	char *buf;
}RBuf;

//1M * 4 * 4 = 16M
#define MAX_SOCKET 1048576

static RBuf rbufs[MAX_RBUFS];

static int mem_total = 0;

#define fd2rbuf(sockfd)  (&rbufs[sockfd])

#define ASSERT_FD(sockfd) if(sockfd <= 0 || sockfd >= MAX_SOCKET){\
                        printf("sockfd(%d) is invalid", sockfd);\
                        return 0;\
                     }

static int rbuf_free(int sockd){
    RBuf *rbuf = fd2rbuf(sockfd);
    return 0;
}

static int rbuf_create(int sockfd, int size){
    RBuf *rbuf = fd2rbuf(sockfd);
    if(buf->buf_len < size){
        if(buf->buf != NULL){
            free(buf->buf);
            buf->buf_len = 0;
            buf->buf = NULL;
            mem_total -= buf->buf_len;
        }
        buf->buf = (char *)malloc(size);
        if(buf->buf == NULL){
            LOG_ERROR("malloc fail");
            return 0;
        }
        buf->buf_len = size;
        mem_total += size;
    }
    buf->rptr = 0;
    buf->wptr = 0;
    return sockfd;
}

static int rbuf_look_int(int sockfd){
    RBuf *rbuf = fd2rbuf(sockfd);
    return *(int *)(rbuf->buf + rbuf->rptr);
}

static int rbuf_remain(int sockfd){
    RBuf *rbuf = fd2rbuf(sockfd);
    return rbuf->buf_len - rbuf->wptr;
}

static int rbuf_len(int sockfd){
    RBuf *rbuf = fd2rbuf(sockfd);
    return rbuf->buf_len;
}

static int rbuf_size(int sockfd){
    RBuf *rbuf = fd2rbuf(sockfd);
    return rbuf->wptr - rbuf->rptr;
}

static char* rbuf_rptr(int sockfd){
    RBuf *rbuf = fd2rbuf(sockfd);
    return rbuf->buf + rbuf->rptr;
}

static char* rbuf_wptr(int sockfd){
    RBuf *rbuf = fd2rbuf(sockfd);
    return rbuf->buf + rbuf->wptr;
}

static int rbuf_rskip(int sockfd, int len){
    RBuf *rbuf = fd2rbuf(sockfd);
    rbuf->rptr += len;
    return len;
}

static int rbuf_wskip(int sockfd, int len){
    RBuf *rbuf = fd2rbuf(sockfd);
    rbuf->wptr += len;
    return len;
}
 
static int rbuf_2line(int sockfd){
    RBuf *rbuf = fd2rbuf(sockfd);
    if(rbuf->rptr == 0)return 0;
    int len = rbuf->wptr - rbuf->rptr;
    memmove(rbuf->buf, rbuf->buf + rbuf->rptr, len);
    rbuf->rptr = 0;
    rbuf->wptr = len;
    return 1;
}

