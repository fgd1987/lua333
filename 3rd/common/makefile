CC=gcc
INCLUDE_DIRS=
LIB_DIRS=
CFLAGS=-fPIC -shared -O2 -Wall 
SO=libcommon.so

${SO}:log.c
	gcc -o ${SO} ${CFLAGS} log.c ${INCLUDE_DIRS} ${LIB_DIRS} 

clean:
	rm -rf ${SO}

install:
	sudo cp -r ${SO} /usr/local/lib

