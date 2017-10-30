# Makefile for building the client/server App in c/c++
# by Michael Prommer and Ralph Quidet

all: clientwin serverwin

clientwin: client/myclient.cpp
	g++ -g -std=c++17 -Wall -o bin/client client/myclient.cpp -lstdc++fs

serverwin: server/myserver.cpp
	g++ -g -std=c++17 -Wall -o bin/server server/myserver.cpp server/server.cpp message/message.cpp -lstdc++fs

clean:
	rm -f bin/client bin/server *.o
