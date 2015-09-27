
CC=gcc
INCLUDE_DIRS=-I../../3rd/lua/src
LIB_DIRS=-L../../lib
CFLAGS=-fPIC -shared -O2 -Wall 
SO=ar.so

${SO}:ar.c
	gcc -o ${SO} ${CFLAGS} ar.c ${INCLUDE_DIRS} ${LIB_DIRS} -llua

clean:
	rm -rf ${SO}

install:
	cp -r ${SO} ../../lib


test:
	../../bin/lua test.lua
