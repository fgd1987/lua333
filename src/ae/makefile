CC=gcc
INCLUDE_DIRS=-I../../3rd/lua/src
LIB_DIRS=-L../../lib
CFLAGS=-fPIC -shared -O1 -Wall 
SO=ae.so

${SO}:lae.c lport.c lrecvbuf.c lsendbuf.c
	gcc -o ${SO} ${CFLAGS} lae.c lport.c ae.c lrecvbuf.c lsendbuf.c ${INCLUDE_DIRS} ${LIB_DIRS} -llua

clean:
	rm -rf ${SO}

install:
	cp -r ${SO} ../../lib
