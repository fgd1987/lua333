
INCLUDE_DIRS=-I../../../3rd/lua/src -I../../../3rd/tolua++
LIB_DIRS=-L../../../lib/ -L../../../3rd/tolua++
CFLAGS=-fPIC -shared -O2 -Wall 
SO=globalsrv.so

${SO}:interface.cc
	g++ -o ${SO} ${CFLAGS} item/item.cc interface.cc ${INCLUDE_DIRS} ${LIB_DIRS} -llua -ltolua

tolua:
	../../../3rd/tolua++/tolua++ -n globalsrv -o interface.cc interface.pkg

test:
	../../../bin/lua test.lua
