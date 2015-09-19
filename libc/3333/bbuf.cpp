
#include "bbuf.h"
#include <stdlib.h>
#include <stdio.h>
#include "log.h"
#include "fdset.h"

#ifndef NULL
#define NULL 0
#endif

#define fd2bbuf(fd) (&bbufs[fd])
typedef struct BBuf_Block{
	struct BBuf_Block *next;
	int buf_size;  //buf的大小 
    int buf_len;   //buf已经使用的大小 
	int rptr;      //读指针
    int unuse;     //字节对齐
	char buf[0];
}BBuf_Block;

typedef struct BBuf{
	BBuf_Block *block_head;
	BBuf_Block *block_tail;
    int used;
}BBuf;

#define MAX_BBUFS 4096 
#define MAX_TABLE 32

static BBuf bbufs[MAX_BBUFS];
static BBuf_Block *block_table[MAX_TABLE];
static fdset_t fdset;

static void bbuf_free_block(BBuf_Block *block);

static int hash(unsigned int size){
	unsigned int index = 0;
	while(((unsigned int)(1<<index)) < size){
        index++;
	}
	return index;
}
void bbuf_stat(){
	int i;
	int sum = 0;
    LOG_STAT("==========SEND BBUF(%d/%d)==========", fdset_top(&fdset), fdset_size(&fdset));
	for(i = 0; i < MAX_TABLE; i++){
		int count = 0;
		BBuf_Block *head = block_table[i];
		while(head){
			count++;
			head = head->next;
		}
		sum += (1<<i)*count;
        if(count > 0){
		    LOG_STAT("%d => %d", i, count);
        }
	}
    sum = sum / 1024;
	LOG_STAT("total mem :%dk", sum);
}

int bbuf_new(int size){
    int fd = fdset_pop(&fdset);
    if(fd == 0)return 0;
	bbufs[fd].block_head = NULL;
	bbufs[fd].block_tail = NULL;
    bbufs[fd].used = 1;
	return fd;
}

int bbuf_free(int fd){
    fdset_push(&fdset, fd);
	BBuf *buf = fd2bbuf(fd);
    if(buf->used == 0)return 0;
    buf->used = 0;
    BBuf_Block *block = buf->block_head;
    while(block != NULL){
        BBuf_Block* next = block->next;
        bbuf_free_block(block);
        block = next;
    }
    return 1;
}
char* bbuf_rptr(int fd){
	BBuf *buf = fd2bbuf(fd);
	if(buf->block_head == NULL){
		return NULL;
	}
	return buf->block_head->buf + buf->block_head->rptr;
}
int bbuf_rlen(int fd){
	BBuf *buf = fd2bbuf(fd);
	if(!buf->block_head){
		return 0;
	}
	return buf->block_head->buf_len - buf->block_head->rptr;
}
int bbuf_rskip(int fd, int s){
	BBuf *buf = fd2bbuf(fd);
    if(buf->block_head == NULL){
        return 0;
    }
	buf->block_head->rptr += s;
	if(buf->block_head->rptr >= buf->block_head->buf_len){
		BBuf_Block *block = buf->block_head;
		buf->block_head = buf->block_head->next;
		if(buf->block_head == NULL){
			buf->block_tail = NULL;
		}
		bbuf_free_block(block);
	}
	return 1;
}
void bbuf_append(int fd, char *buf, int buf_len){
	BBuf *bbuf = fd2bbuf(fd);
    if(bbuf->block_tail == NULL){
        LOG_ERROR("tail is null");
        return;
    }
    if(bbuf->block_tail->buf_size - bbuf->block_tail->buf_len < buf_len){
        LOG_ERROR("size is not enougth %d/%d", bbuf->block_tail->buf_len, bbuf->block_tail->buf_size);
        return;
    }
    bbuf->block_tail->buf_len += buf_len;
}
static void bbuf_free_block(BBuf_Block *block){
    //printf("free block :%d/%d\n", block->buf_len, block->buf_size);
	int idx = hash(block->buf_size + sizeof(BBuf_Block));
	if(block_table[idx] == NULL){
		block->next = NULL;
		block_table[idx] = block;
	}else{
		block->next = block_table[idx];
		block_table[idx] = block;
	}
}
char* bbuf_alloc(int fd, int need_size){
	BBuf *buf = fd2bbuf(fd);
    BBuf_Block *block = buf->block_tail;
    if(block != NULL && block->buf_size - block->buf_len >= need_size){
        return block->buf + block->buf_len;
    }
    //申请一块新的
	int real_size = need_size + sizeof(BBuf_Block);

    //这里加上策略 TODO, buf的大小决定了, 系统调用write的次数和申请的次数
    if(real_size <= 4096){
        real_size = 4096;
    }

	int idx = hash(real_size);
	if(block_table[idx] == NULL){
        int malloc_size = 1<<idx;
        //printf("malloc size: %d/%d\n", need_size, malloc_size - sizeof(BBuf_Block));
		BBuf_Block *block = (BBuf_Block *)malloc(malloc_size);
		if(block == NULL){
            LOG_ERROR("malloc fail");
			return NULL;
		}
		block_table[idx] = block;
		block->next = NULL;
		block->buf_size = malloc_size - sizeof(BBuf_Block);
	}
    //申请失败
    if(block_table[idx] == NULL){
        LOG_ERROR("malloc fail");
        return NULL;
    }
	block = block_table[idx];
	block_table[idx] = block_table[idx]->next;

	block->next = NULL;
	block->rptr = 0;
    block->buf_len = 0;

    if(buf->block_tail == NULL){
        buf->block_tail = buf->block_head = block;
        block->next = NULL;
    }else{
        buf->block_tail->next = block;
        buf->block_tail = block;
        block->next = NULL;
    }
	return block->buf;
}

int bbuf_init(){
    fdset_init(&fdset, MAX_BBUFS);
    return 1;
}
