all:

install:
	cp hiredis/libhiredis* ../lib/
	cp lua/src/lua ../bin/lua
	cp lua/src/liblua.so ../lib/liblua.so
	cp luajit/src/luajit ../bin/luajit
	cp luajit/src/libluajit.so ../lib/libluajit.so
	cp luajit/src/libluajit.so ../lib/liblua.so
	cp luajit/src/luajit ../bin/lua

download:


clean:
	cd hiredis;make clean
	cd protobuf;make clean
	cd redis;make clean
	cd lua;make clean
	cd luajit;make clean
