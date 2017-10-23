#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <map>
#include <sys/types.h>  
#include <sys/stat.h>    
#include <sys/socket.h>       
#include <netinet/in.h>       
#include <arpa/inet.h>        
#include <unistd.h>
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <ctime>
#include <string>
#include <experimental/filesystem>
#include "../Mmessage.h"
#define BUF 100
//#define PORT 6543

using namespace std;

//Server Klasse
class MServer
{
private:
	int create_socket;
	int new_socket;
	socklen_t addrlen;
	char readbuffer[BUF];
	unsigned int port;
	string buffer;
	int size;
	map<int, Mmessage> archive;
	struct sockaddr_in address;
	struct sockaddr_in cliaddress;
	int client_socket;
	int mailcount;
	string directory;
public:
	MServer();
	void startserver(char **argv);
	void savemail(string msg);
	void reading(string username, int nachrichtID);
	void list(string username);
	void del(string username, int nachrichtID);
};


