#include <iostream>
#include <sys/types.h>				
#include <sys/socket.h>				
#include <netinet/in.h>				
#include <arpa/inet.h>				
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../Mmessage.h"
#define BUF 100
//#define PORT 6543

using namespace std;

//Client Klasse
class MClient
{
private:
	int create_socket;
	char readbuffer[BUF];
	string buffer;
	int size;
	unsigned int port;
public:
	MClient(int argc, char **argv);
	void sendmsg(string text);
	void listmsg(string text);
	void readmsg(string textandid);
	void delmsg(string textandid);
	string recvmsg();
	void clearbuffer();
	void closeconnection();
};