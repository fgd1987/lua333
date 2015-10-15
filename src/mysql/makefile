CC=g++

ifeq ($(shell getconf LONG_BIT), 64)
INCLUDE_DIRS=-I../../3rd/lua/src -I/usr/include/mysql -I../../3rd/common
LIB_DIRS=-L../../lib/ -L/usr/lib64/mysql/ -L../../3rd/common
CFLAGS=-fPIC -shared -O2 -Wall 
SO=mysql.so
endif

ifeq ($(shell uname), Darwin)
INCLUDE_DIRS=-I../../3rd/lua/src -I../../3rd/common -I../../3rd/mysql-connector-c64-mac/include
LIB_DIRS=-L../../lib -L../../3rd/common -L../../3rd/mysql-connector-c64-mac/lib
CFLAGS=-fPIC -shared -O2 -Wall 
SO=mysql.so
endif

${SO}:lmysql.c
	g++ -o ${SO} ${CFLAGS} lmysql.c ${INCLUDE_DIRS} ${LIB_DIRS} -llua -lmysqlclient -lcommon

clean:
	rm -rf ${SO}

install:
	cp -r ${SO} ../../lib

test:
	../../bin/lua test.lua
