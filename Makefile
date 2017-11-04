# Makefile for client/server in c++
# by Ulrich Gram

all: clientwin serverwin

clientwin: client/myclient.cpp
	g++ -g -std=c++17 -Wall -o bin/client client/myclient.cpp -lstdc++fs

serverwin: server/myserver.cpp
	g++ -g -std=c++17 -Wall -o bin/server server/myserver.cpp server/server.cpp message/message.cpp -lstdc++fs -pthread -lldap -llber -DLDAP_DEPRECATED

clean:
	rm -f bin/client bin/server *.o
