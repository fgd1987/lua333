

all:
	gcc -o tolua++ tolua.c toluabind.c tolua_event.c tolua_is.c tolua_map.c tolua_push.c tolua_to.c -llua -I../lua/src -L../../lib

test:
	./tolua++ -n gamesrv -o testfile/interface.cc testfile/interface.pkg 

so:
	gcc -o libtolua.so -fPIC -shared -O2 -Wall  toluabind.c tolua_event.c tolua_is.c tolua_map.c tolua_push.c tolua_to.c -llua -I../lua/src -L../../lib



