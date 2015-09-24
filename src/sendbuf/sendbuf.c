#include <stdlib.h>
#include <stdio.h>

/* 
 * rptr rlen rskip 配合使用，用于写socket
 * rptr = rptr()
 * rlen = rlen()
 * sent = send(sockfd, rptr, rlen)
 * rskip(sent)
 *
 * append 和 alloc配合使用, 用于写buff
 * buf = alloc(sockfd, size)
 * len = writeto(buf)
 * append(sockfd, buf, len)
 * /
 

#ifndef NULL
#define NULL 0
#endif

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
}BBuf;

#define MAX_SOCKET 65536
#define MAX_TABLE 32

static BBuf bbufs[MAX_SOCKET];
static BBuf_Block *block_table[MAX_TABLE];
#define fd2bbuf(sockfd) (&bbufs[sockfd])

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
    printf("==========SEND BBUF==========\n");
	for(i = 0; i < MAX_TABLE; i++){
		int count = 0;
		BBuf_Block *head = block_table[i];
		while(head){
			count++;
			head = head->next;
		}
		sum += (1<<i)*count;
        if(count > 0){
		    printf("%d => %d\n", i, count);
        }
	}
    sum = sum / 1024;
	printf("total mem :%dk\n", sum);
}

int bbuf_create(int sockfd, int size){
	bbufs[sockfd].block_head = NULL;
	bbufs[sockfd].block_tail = NULL;
    return 0;
}

int bbuf_free(int sockfd){
	BBuf *buf = fd2bbuf(sockfd);
    BBuf_Block *block = buf->block_head;
    while(block != NULL){
        BBuf_Block* next = block->next;
        bbuf_free_block(block);
        block = next;
    }
    return 0;
}

char* bbuf_rptr(int sockfd){
	BBuf *buf = fd2bbuf(sockfd);
	if(buf->block_head == NULL){
		return NULL;
	}
	return buf->block_head->buf + buf->block_head->rptr;
}

int bbuf_rlen(int sockfd){
	BBuf *buf = fd2bbuf(sockfd);
	if(!buf->block_head){
		return 0;
	}
	return buf->block_head->buf_len - buf->block_head->rptr;
}

static int bbuf_rskip(int sockfd, int s){
	BBuf *buf = fd2bbuf(sockfd);
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

static int bbuf_append(int sockfd, char *buf, int buf_len){
	BBuf *bbuf = fd2bbuf(sockfd);
    if(bbuf->block_tail == NULL){
        printf("tail is null\n");
        return 1;
    }
    if(bbuf->block_tail->buf_size - bbuf->block_tail->buf_len < buf_len){
        printf("size is not enougth %d/%d\n", bbuf->block_tail->buf_len, bbuf->block_tail->buf_size);
        return 2;
    }
    bbuf->block_tail->buf_len += buf_len;
}

char* bbuf_alloc(int sockfd, int need_size){
	BBuf *buf = fd2bbuf(sockfd);
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
            printf("malloc fail\n");
			return NULL;
		}
		block_table[idx] = block;
		block->next = NULL;
		block->buf_size = malloc_size - sizeof(BBuf_Block);
	}
    //申请失败
    if(block_table[idx] == NULL){
        printf("malloc fail\n");
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
