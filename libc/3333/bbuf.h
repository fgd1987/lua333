#ifndef _BBUF_H_
#define _BBUF_H_


/*
 *
 * int fd = bbuf_new(2048);
 *
 * char *buf = bbuf_alloc(fd, 1024);
 * strcpy(buf, "hello")
 * bbuf_append(fd, buf, 6);
 *
 *	char *ptr = bbuf_ptr(fd);
 *	int len = bbuf_len(fd);
 *	len = write(socket, ptr, len);
 *	if(len > 0){
 *		bbuf_skip(fd, len);
 *	}
 *
 *
 */

int bbuf_new(int size);
int bbuf_free(int fd);
char* bbuf_alloc(int fd, int size);

void bbuf_stat();

void bbuf_append(int fd, char *buf, int buf_size);
int bbuf_rskip(int fd, int s);
int bbuf_rlen(int fd);
char* bbuf_rptr(int fd);

void bbuf_info();


int bbuf_init();
#endif
