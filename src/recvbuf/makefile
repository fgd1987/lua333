CC=gcc
INCLUDE_DIRS=-I../../3rd/lua/src
LIB_DIRS=-L../../libs/lua/src
LIB_DIRS=-L/opt/local/lib
CFLAGS=-fPIC -shared -O2 -Wall 
SO=recvbuf.so

${SO}:recvbuf.c
	gcc -o ${SO} ${CFLAGS} recvbuf.c ${INCLUDE_DIRS} ${LIB_DIRS} -llua

clean:
	rm -rf ${SO}

install:
	cp -r ${SO} ../../lib
