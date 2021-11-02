COMPILE_DEBUG = -Wall -ggdb --std=c++11
COMPILE_RELEASE = -O3 --std=c++11
COMP_ARGS = $(COMPILE_DEBUG)

all: tunnel dclient

tunnel: tunnel.o
	$(CXX) -lpthread -lcrypto -lssl -o tunnel_server \
		main.o util.o tls_server.o

tunnel.o:
	$(CXX) $(COMP_ARGS) -I aixlog/include -c \
		main.cpp tls_server.cpp util.cpp

dclient: dclient.o
	$(CXX) -lpthread -lcrypto -lssl -o dclient debug_client.o

dclient.o:
	$(CXX) $(COMP_ARGS) -c debug_client.cpp

clean:
	rm -f tunnel_server dclient tls_server.o main.o util.o debug_client.o
