CC=g++
INCLUDE_DIRS=-I../../3rd/lua/src -I../../3rd/common
LIB_DIRS=-L../../lib/ -L../../3rd/common
CFLAGS=-fPIC -shared -O2 -Wall 
SO=system.so

${SO}:lsystem.c
	g++ -o ${SO} ${CFLAGS} lsystem.c ${INCLUDE_DIRS} ${LIB_DIRS} -llua -lcommon

clean:
	rm -rf ${SO}

install:
	cp -r ${SO} ../../lib


test:
	../../bin/lua test.lua
