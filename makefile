
install:
	cp 3rd/common/libcommon.so /usr/local/lib/libcommon.so
	cp 3rd/lua/src/lua bin/lua
	cp 3rd/lua/src/lua /usr/local/bin/lua
	cp 3rd/lua/src/liblua.so /usr/local/lib/liblua.so
	cp 3rd/lua/src/liblua.so /usr/lib/liblua.so
	cp 3rd/common/libcommon.so /usr/local/lib
	cp 3rd/common/libcommon.so /usr/lib
	cp 3rd/tolua++/libtolua.so /usr/lib
	cp bin/luaenv /usr/local/bin/luaenv
	cp bin/luaexe /usr/local/bin/luaexe
	cp /usr/local/lib/libprotobuf.so lib/libprotobuf.a
