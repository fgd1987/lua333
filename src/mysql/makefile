CC=gcc

ifeq ($(shell getconf LONG_BIT), 64)
INCLUDE_DIRS=-I../../3rd/lua/src -I/usr/include/mysql
LIB_DIRS=-L../../lib/ -L/usr/lib64/mysql/
CFLAGS=-fPIC -shared -O2 -Wall 
SO=mysql.so
endif

ifeq ($(shell uname), Darwin)
INCLUDE_DIRS=-I../../3rd/lua/src -I../../3rd/mysql-connector-c64-mac/include
LIB_DIRS=-L../../lib/ 
CFLAGS=-fPIC -shared -O2 -Wall 
SO=mysql.so
endif

${SO}:lmysql.c
	gcc -o ${SO} ${CFLAGS} lmysql.c ${INCLUDE_DIRS} ${LIB_DIRS} -llua -lmysqlclient

clean:
	rm -rf ${SO}

install:
	cp -r ${SO} ../../lib

test:
	../../bin/lua test.lua
