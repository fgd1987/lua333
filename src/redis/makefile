CC=gcc
INCLUDE_DIRS=-I../../3rd/lua/src -I../../3rd/redis/src -I../../3rd/hiredis
LIB_DIRS=-L../../lib/
CFLAGS=-fPIC -shared -O2 -Wall 
SO=redis.so

${SO}:lredis.c
	gcc -o ${SO} ${CFLAGS} lredis.c ../../3rd/hiredis/libhiredis.a ${INCLUDE_DIRS} ${LIB_DIRS} -llua 

clean:
	rm -rf ${SO}

install:
	cp -r ${SO} ../../lib

test:
	../../bin/lua test.lua
