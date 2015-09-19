#ifndef _FD_SET_H_
#define _FD_SET_H_


typedef struct fdset_t{
    int top;
    int size;    
    int *stack;
}fdset_t;


int fdset_init(fdset_t *self, int size);
int fdset_pop(fdset_t *self);
int fdset_push(fdset_t *self, int fd);

#define fdset_top(self) ((self)->top)
#define fdset_size(self) ((self)->size)

#endif
