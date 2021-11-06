COMPILE_DEBUG = -Wall -ggdb --std=c++11
COMPILE_RELEASE = -O3 --std=c++11
COMP_ARGS = $(COMPILE_DEBUG)

TUNSRV_CARGS = $(COMP_ARGS) -I aixlog/include
TUNSRV_LARGS = -lpthread -lcrypto -lssl
TUNSRV_OBJ = main.o tls_server.o util.o
TUNSRV_BIN = tunnel_server

all: tunnel dclient

tunnel: main.cxx tls_server.cxx util.cxx
	$(CXX) $(TUNSRV_LARGS) $(TUNSRV_OBJ) -o $(TUNSRV_BIN)

main.cxx:
	$(CXX) $(TUNSRV_CARGS) -c main.cpp

tls_server.cxx:
	$(CXX) $(TUNSRV_CARGS) -c util/tls_server.cpp

util.cxx:
	$(CXX) $(TUNSRV_CARGS) -c util/util.cpp

debug_client: debug_client.cpp
	$(CXX) debug_client.o -o dclient

debug_client.cpp:
	$(CXX) $(TUNSRV_CARGS) -c debug_client.cpp

clean:
	rm -f $(TUNSRV_BIN) dclient $(TUNSRV_OBJ) debug_client.o
