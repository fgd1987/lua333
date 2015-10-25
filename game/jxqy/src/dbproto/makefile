ENGINE_PATH=../../../..
INCLUDE_DIRS=-I$(ENGINE_PATH)/3rd/lua/src -I$(ENGINE_PATH)/3rd/common -I$(ENGINE_PATH)/3rd/tolua++ -I$(ENGINE_PATH)/3rd/protobuf/src -I../
LIB_DIRS=-L$(ENGINE_PATH)/3rd/lua/src -L$(ENGINE_PATH)/3rd/common -L$(ENGINE_PATH)/3rd/tolua++ -L/usr/local/lib
CFLAGS=-fPIC -shared -O2 -Wall -lprotobuf
SO=dbproto.so
SRC=player.pb.cc achieve.pb.cc friend.pb.cc task.pb.cc user.pb.cc interface.cc dbproto.cc

$(SO):$(SRC)
	echo '编译'
	g++ -o $(SO) $(CFLAGS) $(SRC) $(INCLUDE_DIRS) $(LIB_DIRS) -llua -ltolua -lcommon

tolua:
	echo '执行tolua++'
	$(ENGINE_PATH)/3rd/tolua++/tolua++ -n dbproto -o interface.cc dbproto.pkg

protoc:
	echo '执行protoc'
	cd ../..;protoc dbproto/* --cpp_out=src

protolua:
	echo '执行protolua'
	$(ENGINE_PATH)/bin/protoc ../../dbproto/* -n dbproto -o .

test:
	$(ENGINE_PATH)/bin/luaexe test.lua

clean:
	rm -rf $(SO)


