
#include "bbuf.h"
#include <stdio.h>
#include <string.h>
int main(int argc, char **argv){

    int fd = bbuf_new(2048);
    char *buf = bbuf_alloc(fd, 20);
    strcpy(buf, "hello");
    bbuf_append(fd, buf, 6);

    buf = bbuf_alloc(fd, 2038);
    strcpy(buf, "hello");
    bbuf_append(fd, buf, 6);

    buf = bbuf_alloc(fd, 20);
    strcpy(buf, "hello");
    bbuf_append(fd, buf, 6);

    buf = bbuf_alloc(fd, 20);
    strcpy(buf, "hello");
    bbuf_append(fd, buf, 6);

    char *ptr;
    while(ptr = bbuf_ptr(fd)){
        int rlen = bbuf_len(fd);
        printf("len :%d\n", rlen);
        printf("%s\n", ptr);
        bbuf_skip(fd, rlen);
    }

}
