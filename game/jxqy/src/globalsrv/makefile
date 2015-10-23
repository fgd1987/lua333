
ENGINE_PATH=../../../../
INCLUDE_DIRS=-I$(ENGINE_PATH)/3rd/lua/src -I$(ENGINE_PATH)/3rd/common -I$(ENGINE_PATH)/3rd/tolua++
LIB_DIRS=-L$(ENGINE_PATH)/3rd/lua/src -L$(ENGINE_PATH)3rd/common -L$(ENGINE_PATH)/3rd/tolua++
CFLAGS=-fPIC -shared -O2 -Wall 
SO=globalsrv.so

${SO}:interface.cc
	g++ -o ${SO} ${CFLAGS} item/item.cc interface.cc ${INCLUDE_DIRS} ${LIB_DIRS} -llua -ltolua -lcommon

tolua:
	$(ENGINE_PATH)/3rd/tolua++/tolua++ -n globalsrv -o interface.cc interface.pkg

test:
	$(ENGINE_PATH)/bin/luaexe test.lua

clean:
	rm -rf ${SO}
