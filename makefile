

install:
	sudo cp bin/lua /usr/local/bin/lua
	sudo cp bin/luajit /usr/local/bin/luajit
	sudo cp bin/luaenv /usr/local/bin/luaenv
	sudo cp 3rd/common/libcommon.so /usr/local/lib
	sudo cp 3rd/common/libcommon.so /usr/lib
	sudo cp /usr/local/lib/libprotobuf.so lib/libprotobuf.a
