COMPILE_DEBUG = -Wall -ggdb --std=c++11
COMPILE_RELEASE = -O3 --std=c++11
COMP_ARGS = $(COMPILE_DEBUG)

TUNSRV_CARGS = $(COMP_ARGS) -I. -I clue/include
TUNSRV_LARGS = -lpthread -lcrypto -lssl
TUNSRV_OBJ = main.o tls_server.o util.o router.o tunnel_endpoints.o user.o uuid_token.o
TUNSRV_BIN = tunnel_server

all: tunnel debug_client

tunnel: main.cxx tls_server.cxx util.cxx router.cxx tunnel_endpoints.cxx user.cxx uuid_token.cxx
	$(CXX) $(TUNSRV_LARGS) $(TUNSRV_OBJ) -o $(TUNSRV_BIN)

main.cxx:
	$(CXX) $(TUNSRV_CARGS) -c main.cpp


router.cxx:
	$(CXX) $(TUNSRV_CARGS) -c tunnel/router.cpp

tunnel_endpoints.cxx:
	$(CXX) $(TUNSRV_CARGS) -c tunnel/tunnel_endpoints.cpp

user.cxx:
	$(CXX) $(TUNSRV_CARGS) -c tunnel/user.cpp

uuid_token.cxx:
	$(CXX) $(TUNSRV_CARGS) -c tunnel/uuid_token.cpp


tls_server.cxx:
	$(CXX) $(TUNSRV_CARGS) -c util/tls_server.cpp

util.cxx:
	$(CXX) $(TUNSRV_CARGS) -c util/util.cpp


debug_client: debug_client.cxx
	$(CXX) -lpthread -lssl -lcrypto debug_client.o -o dclient

debug_client.cxx:
	$(CXX) $(TUNSRV_CARGS) -c debug_client.cpp

clean:
	rm -f $(TUNSRV_BIN) dclient $(TUNSRV_OBJ) debug_client.o
