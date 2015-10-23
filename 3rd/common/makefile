CC=g++
INCLUDE_DIRS=
LIB_DIRS=
CFLAGS=-fPIC -shared -O2 -Wall 
SO=libcommon.so

${SO}:log.c cstr.h cstr.cc hash.h hash.cc array.h array.cc object.h object.cc
	g++ -o ${SO} ${CFLAGS} log.c cstr.cc hash.cc object.cc array.cc ${INCLUDE_DIRS} ${LIB_DIRS} 
	g++ -o test -fPIC -O2 -Wall test.cc -L. -lcommon 

clean:
	rm -rf ${SO}
	rm -rf test

install:
	sudo cp -r ${SO} /usr/local/lib
	sudo cp -r ${SO} /usr/lib

