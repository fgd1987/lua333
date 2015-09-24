CC=gcc
include_dirs=-I../../libs/lua/src
lib_dirs=-L../../libs/lua/src
CFLAGS=-fPIC -shared -O2 -Wall 
select.o:
	gcc ${CFLAGS} select.c ${include_dirs} ${lib_dirs} -llua
