
CC=gcc
INCLUDE_DIRS=-I../../libs/lua/src
LIB_DIRS=-L../../libs/lua/src
LIB_DIRS=-L/opt/local/lib
CFLAGS=-fPIC -shared -O2 -Wall 
SO=ar.so

${SO}:
	gcc -o ${SO} ${CFLAGS} ar.c ${INCLUDE_DIRS} ${LIB_DIRS} -llua

clean:
	rm -rf ${SO}


