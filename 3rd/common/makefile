CC=g++
INCLUDE_DIRS=
LIB_DIRS=
CFLAGS=-fPIC -shared -O2 -Wall 
SO=libcommon.so

${SO}:log.c
	g++ -o ${SO} ${CFLAGS} $^ ${INCLUDE_DIRS} ${LIB_DIRS} 

test:
	g++ -o test -fPIC -O2 -Wall test.cc -L. -lcommon 

clean:
	rm -rf ${SO}
	rm -rf test

install:
	sudo cp -r ${SO} /usr/local/lib
	sudo cp -r ${SO} /usr/lib

