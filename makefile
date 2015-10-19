

install:
	cp 3rd/common/libcommon.so /usr/local/lib/libcommon.so
	cp 3rd/lua/src/lua bin/lua
	cp 3rd/lua/src/lua /usr/local/bin/lua
	cp 3rd/lua/src/liblua.so lib/liblua.so
	cp 3rd/lua/src/liblua.so /usr/local/lib/liblua.so
	cp 3rd/luajit/src/luajit bin/luajit
	cp 3rd/luajit/src/libluajit.so lib/libluajit.so
	cp 3rd/luajit/src/libluajit.so /usr/local/lib/libluajit.so
	cp 3rd/common/libcommon.so /usr/local/lib
	cp bin/luaenv /usr/local/bin/luaenv
	cp /usr/local/lib/libprotobuf.so lib/libprotobuf.a
