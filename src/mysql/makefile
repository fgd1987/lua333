CC=gcc
INCLUDE_DIRS=-I../../3rd/lua/src -I../../3rd/mysql-connector-c64-mac/include
LIB_DIRS=-L../../lib/
CFLAGS=-fPIC -shared -O2 -Wall 
SO=mysql.so

${SO}:lmysql.c
	gcc -o ${SO} ${CFLAGS} lmysql.c ${INCLUDE_DIRS} ${LIB_DIRS} -llua -lmysqlclient

clean:
	rm -rf ${SO}

install:
	cp -r ${SO} ../../lib

test:
	../../lib/lua test.lua
