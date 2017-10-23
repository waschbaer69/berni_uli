#include "Mclient.h"

MClient::MClient(int argc, char **argv)
{
  //Socketerstellung vom Clienten im Konstruktor
  struct sockaddr_in address;

  /*	
  		IP-Adresse wird im Konstruktor übergeben, bei falscher Eingabe 
  		wird dem Benutzer die Usage angezeigt
  */
  if( argc < 3 ){
     printf("Usage: %s ServerAdresse PORT\n", argv[0]);
     exit(EXIT_FAILURE);
  }

  //Falls keine Verbindung -> ERROR
  if ((create_socket = socket (AF_INET, SOCK_STREAM, 0)) == -1)
  {
     perror("Socket error");
     throw exception();
  }

  //Port wird übergeben und adress wird geleert
  port = stoi(argv[2]);
  memset(&address,0,sizeof(address));

  //im struct adress werden sin_family und port gesetzt (htons networkbyteorder)
  address.sin_family = AF_INET;
  address.sin_port = htons (port);

  inet_aton (argv[1], &address.sin_addr);

  //Falls Verbindung aufgebaut wird -> wird der Benutzer informiert
  //Information wird ausgegeben
  if (connect ( create_socket, (struct sockaddr *) &address, sizeof (address)) == 0)
  {
     printf ("You are now logged in. (%s)\n", inet_ntoa (address.sin_addr));
     size=recv(create_socket,readbuffer,BUF-1, 0);
     if (size>0)
     {
      	buffer = readbuffer;
        buffer[size]= '\0';
        cout << buffer << endl;
        clearbuffer();
     }
  }
  else  
  {     
    perror("Connect error - no server available");
    throw exception();
  }
}; 

void MClient::sendmsg(string text)
{
  send(this->create_socket, text.c_str(), text.length(), 0);
}

void MClient::listmsg(string text)
{
  send(this->create_socket, text.c_str(), text.length(), 0);
}

void MClient::readmsg(string textandid)
{
  send(this->create_socket, textandid.c_str(), textandid.length(), 0);
}

void MClient::delmsg(string textandid)
{
  send(this->create_socket, textandid.c_str(), textandid.length(), 0);
}

/*	Falls eine größere Menge an Daten geschickt wird,
	wird der readbuffer-Inhalt in Stücken an einen String gehängt
	(bis die Nachricht vollständig angekommen ist)
*/
string MClient::recvmsg()
{
  while((size = recv(create_socket, readbuffer,BUF-1, 0)) > 0)
      {
        buffer.append(readbuffer, size);
        if(size < BUF-1) break;
      }
  string tmp = buffer;
  buffer = "";
  return tmp;
}

void MClient::clearbuffer()
{
  buffer = "";
  memset(&readbuffer, 0, sizeof(readbuffer));
}

void MClient::closeconnection()
{
  close (create_socket);
}


