CC=g++
INCLUDE_DIRS=-I../../3rd/lua/src -I../../3rd/common -I../../3rd/protobuf/src
LIB_DIRS=-L../../lib/ -L../../3rd/common ../../lib/libprotobuf.a
CFLAGS=-fPIC -shared -O2 -Wall 
SO=pbc.so

${SO}:pbc.cpp
	${CC} -o ${SO} ${CFLAGS} pbc.cpp ${INCLUDE_DIRS} ${LIB_DIRS} -llua -lcommon

clean:
	rm -rf ${SO}

install:
	cp -r ${SO} ../../lib

test:
	../../bin/lua test.lua
