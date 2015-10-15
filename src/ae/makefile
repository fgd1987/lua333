CC=g++
INCLUDE_DIRS=-I../../3rd/lua/src -I../../3rd/common
LIB_DIRS=-L../../lib -L../../3rd/common
CFLAGS=-fPIC -shared -O1 -Wall 
SO=ae.so

${SO}:lae.c lport.c lrecvbuf.c lsendbuf.c ltimer.c
	g++ -o ${SO} ${CFLAGS} lae.c lport.c ae.c lrecvbuf.c lsendbuf.c ltimer.c ${INCLUDE_DIRS} ${LIB_DIRS} -llua -lcommon

clean:
	rm -rf ${SO}

install:
	cp -r ${SO} ../../lib
