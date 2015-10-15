CC=g++
INCLUDE_DIRS=-I../../3rd/lua/src
LIB_DIRS=-L../../lib/
CFLAGS=-fPIC -shared -O2 -Wall 
SO=stringpatch.so

${SO}:lstringpatch.c md5.c
	g++ -o ${SO} ${CFLAGS} md5.c lstringpatch.c ${INCLUDE_DIRS} ${LIB_DIRS} -llua 

clean:
	rm -rf ${SO}

install:
	cp -r ${SO} ../../lib


test:
	../../bin/lua test.lua
