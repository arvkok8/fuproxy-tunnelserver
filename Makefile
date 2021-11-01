all: tunnel

tunnel: tunnel.o
	$(CXX) -lpthread -lcrypto -lssl -o tunnel_server \
		main.o util.o tls_server.o

tunnel.o:
	$(CXX) -Wall -ggdb --std=c++11 -I aixlog/include -c \
		main.cpp tls_server.cpp util.cpp

clean:
	rm -f tunnel_server tls_server.o main.o util.o
