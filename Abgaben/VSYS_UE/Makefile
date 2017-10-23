# Makefile for building the client/server App in c/c++
# by Michael Prommer and Ralph Quidet

all: clientwin serverwin

clientwin: client/Mclient.cpp
	g++ -g -std=c++17 -Wall -o bin/client client/main.cpp client/Mclient.cpp Mmessage.cpp -lstdc++fs

serverwin: server/Mserver.cpp
	g++ -g -std=c++17 -Wall -o bin/server server/main.cpp server/Mserver.cpp Mmessage.cpp -lstdc++fs

clean:
	rm -f bin/client bin/server *.o
