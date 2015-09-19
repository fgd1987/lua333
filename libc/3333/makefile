TARGET:=lualua
all: $(TARGET)

.PHONY:all clean install

CFLAGS=-g -O3 -Wall 

CC=gcc
CXX=g++

ifeq ($(shell getconf LONG_BIT), 32)
	INCLUDE=-I. -I./lua/src -I./hiredis -I./mysql-connector-c32/include
	LIBS=-lm -lprotobuf -ldl -lssl ./hiredis/libhiredis.a ./lua/src/liblua.a ./mysql-connector-c32/lib/libmysqlclient.a
	LIBS_DIR=-L./hiredis -L./mysql-connector-c32/lib
else
	INCLUDE=-I. -I./lua/src -I./hiredis -I./mysql-connector-c64/include
	LIBS=-lm -lprotobuf -ldl -lssl ./hiredis/libhiredis.a ./lua/src/liblua.a ./mysql-connector-c64/lib/libmysqlclient.a
	LIBS_DIR=-L./hiredis -L./mysql-connector-c64/lib
endif
ifeq ($(shell uname), Darwin)
	INCLUDE=-I. -I./lua/src -I./hiredis -I./mysql-connector-c64-mac/include
	LIBS=-lm -lprotobuf -ldl -lssl ./hiredis/libhiredis.a ./lua/src/liblua.a ./mysql-connector-c64-mac/lib/libmysqlclient.a
	LIBS_DIR=-L./hiredis -L./mysql-connector-c64-mac/lib
endif

CXXOBJECTS=evt.o luas.o  ar.o log.o rbuf.o cJSON.o json.o bbuf.o rmi_port.o fdset.o pb_socket.o linenoise.o\
	zmalloc.o os.o sys.o port.o timer.o main.o lualua.o  pbc.o pb_port.o ae.o  redis.o lmysql.o utils.o\
	   raw_port.o bson_encode.o cBson.o md5.o queue.o
COBJECTS=bson.o encoding.o numbers.o

CXXSOURCE=$(CXXOBJECTS:.o=.cpp)
CSOURCE=$(COBJECTS:.o=.c)


$(TARGET):$(COBJECTS) $(CXXOBJECTS)
	make -C hiredis 
	make -C lua linux
	$(CXX) -o $@ $(CXXOBJECTS) $(COBJECTS) $(LIBS)

$(CXXOBJECTS):%.o:%.cpp
	$(CXX) $(CFLAGS) $(LIBS_DIR) $(INCLUDE) -c $<  

$(COBJECTS):%.o:%.c
	$(CC) $(CFLAGS) $(LIBS_DIR) $(INCLUDE) -c $< 


clean:
	rm -rf *.o
	rm -rf lualua
install:
	cp lualua ../bin/lualua
