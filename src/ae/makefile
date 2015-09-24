CC=gcc
INCLUDE_DIRS=-I../../3rd/lua/src
LIB_DIRS=-L../../3rd/lua/src
LIB_DIRS=-L/opt/local/lib
CFLAGS=-fPIC -shared -O2 -Wall 
SO=ae.so

${SO}:lae.c
	gcc -o ${SO} ${CFLAGS} lae.c ae.c ${INCLUDE_DIRS} ${LIB_DIRS} -llua

clean:
	rm -rf ${SO}

install:
	cp -r ${SO} ../../lib
