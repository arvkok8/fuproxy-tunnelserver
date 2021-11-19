COMPILE_DEBUG = -Wall -ggdb --std=c++11
COMPILE_RELEASE = -O3 --std=c++11
COMP_ARGS = $(COMPILE_DEBUG)

TUNSRV_CARGS = $(COMP_ARGS) -I. -I clue/include -I cppcodec/
TUNSRV_LARGS = -lpthread -lcrypto -lssl

TUNNEL_SRC = $(wildcard tunnel/*.cpp)
TUNNEL_OBJ = $(patsubst %.cpp,%.o,$(TUNNEL_SRC))
UTIL_SRC = $(wildcard util/*.cpp)
UTIL_OBJ = $(patsubst %.cpp,%.o,$(UTIL_SRC))

TUNSRV_BIN = tunnel_server

all: $(TUNSRV_BIN) debug_client

$(TUNSRV_BIN): main.o $(TUNNEL_OBJ) $(UTIL_OBJ)
	$(CXX) $^ -o $(TUNSRV_BIN) $(TUNSRV_LARGS)

$(TUNNEL_OBJ): tunnel/%.o: tunnel/%.cpp
	$(CXX) $(TUNSRV_CARGS) -c $< -o $(patsubst %.cpp,%.o,$<)

$(UTIL_OBJ): util/%.o: util/%.cpp
	$(CXX) $(TUNSRV_CARGS) -c $< -o $(patsubst %.cpp,%.o,$<)

main.o:
	$(CXX) $(TUNSRV_CARGS) -c main.cpp -o main.o

debug_client: debug_client.o
	$(CXX) $(TUNSRV_LARGS) debug_client.cpp -o debug_client

debug_client.o: debug_client.cpp
	$(CXX) $(TUNSRV_CARGS) -c debug_client.cpp

clean: clean_tunnel_server clean_debug_client

clean_tunnel_server:
	rm -f $(TUNSRV_BIN) $(TUNNEL_OBJ) $(UTIL_OBJ) main.o

clean_debug_client:
	rm -f debug_client debug_client.o
