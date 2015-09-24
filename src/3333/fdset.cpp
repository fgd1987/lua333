
#include "fdset.h"

#include <stdio.h>
#include <stdlib.h>


int fdset_init(fdset_t *self, int size){
    self->stack = (int *)malloc(sizeof(int) * size);
    if(self == NULL || size == 0){
        printf("fdset_init fail\n");
        return 0;
    }
    self->size = size;
    self->top = size - 1;
    int fd = 1;
    for(int i = size - 1; i >= 0; i--){
        self->stack[i] = fd++;
    }
    return 1;
}
int fdset_pop(fdset_t *self){
    if(self->top == 0){
        return 0;
    }
    return self->stack[self->top--];
}
int fdset_push(fdset_t *self, int fd){
    if(fd <= 0 || fd >= self->size){
        return 0;
    }
    if(self->top == self->size - 1){
        printf("fdset_push fail\n");
        return 0;
    }
    self->top++;
    self->stack[self->top] = fd;
    return 1;
}
