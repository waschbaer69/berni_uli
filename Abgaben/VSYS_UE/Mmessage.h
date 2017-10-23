#include <iostream>
#include <sys/types.h>				
#include <sys/socket.h>				
#include <netinet/in.h>				
#include <arpa/inet.h>				
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sstream>
#include <string>
using namespace std;

//Message Klasse, um Email zu versenden
class Mmessage
{
private:
	string sender;
	string empf;
	string betreff;
	string nachricht;
	int nachrichtID;
	string fullmsg;
public:
	Mmessage(string text);

	//getter-methods
	string get_sender();
	string get_empf();
	string get_betreff();
	string get_nachricht();
};
