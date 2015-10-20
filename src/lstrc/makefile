CC=g++
INCLUDE_DIRS=-I../../3rd/lua/src 
LIB_DIRS=-L../../lib/
CFLAGS=-fPIC -shared -O2 -Wall 
SO=actordb.so

all:actordb.so


clean:
	rm -rf ${SO}

install:
	cp -r ${SO} ../../lib

build:
	./lstrc actordb.scp actordb.h

actordb.so:actordb.h actordb.cc
	g++ -o $@ ${CFLAGS} actordb.cc ${INCLUDE_DIRS} ${LIB_DIRS} -llua 

test:
	../../bin/lua test.lua
