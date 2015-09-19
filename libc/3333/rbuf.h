#ifndef _SBUF_H_
#define _SBUF_H_

#include "os.h"


int rbuf_init();

int rbuf_free(int buffd);

int rbuf_new(int size);



int rbuf_remain(int buffd);
int rbuf_size(int buffd);
int rbuf_len(int buffd);

char* rbuf_rptr(int buffd);
char* rbuf_wptr(int buffd);

int rbuf_rskip(int buffd, int len);
int rbuf_wskip(int buffd, int len);

int rbuf_2line(int buffd);

int rbuf_look_int(int buffd);
void rbuf_stat();
#endif
