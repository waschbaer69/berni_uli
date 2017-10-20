/* myclient.c */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define BUF 1024

/* Erhält Message vom Server und gibt sie aus */
void receiveMessage(int socket, char* buffer)
{
  int size=recv(socket,buffer,BUF-1, 0);
  if (size>0)
  {
    buffer[size]= '\0';
    printf("%s",buffer);
  }
}

/* Lest von der Tastatur und sendet an Server */
void sendMessage(int socket, char* buffer)
{
  fgets (buffer, BUF, stdin);
  send(socket, buffer, strlen (buffer), 0);
}


void sendTillOK(int socket, char* buffer, char* text)
{
  do
  {
    printf("%s: ",text);
    sendMessage(socket,buffer);
    receiveMessage(socket,buffer); // Receive OK or ERR
  }
  while(strcmp("OK\n",buffer) != 0);
}

void sendProtocol(int socket, char *buffer)
{
  printf("SEND:\n");
  printf("=============================================================\n");
  sendTillOK(socket, buffer, "Sender");
  sendTillOK(socket, buffer, "Receiver");
  sendTillOK(socket, buffer, "Subject");
  // content
  printf("Content: (end with '.') \n");
  while(strcmp(buffer,".\n") != 0)
  {
    sendMessage(socket,buffer);
  }
  printf("=============================================================\n");
}

void readProtocol(int socket, char *buffer)
{
  printf("SEND:\n");
  printf("=============================================================\n");
  sendTillOK(socket, buffer, "Username");
  printf("Message-Number:");
  sendMessage(socket,buffer);
  receiveMessage(socket,buffer);
  printf("=============================================================\n");
}

int main (int argc, char **argv) {

  int create_socket;

  char buffer[BUF]; // Buffer 1024

  struct sockaddr_in address;
  int size;

  if( argc < 3){
    printf("Usage: %s IP_address Port\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  if ((create_socket = socket (AF_INET, SOCK_STREAM, 0)) == -1)
  {
    perror("Socket error");
    return EXIT_FAILURE;
  }

  memset(&address,0,sizeof(address));  // Adressstruktur mit 0 initialisieren
  address.sin_family = AF_INET; // IPv4
  address.sin_port = htons (atoi(argv[2])); // angegebenen Port (argv[2]) in integer (atoi) und host-to-network für short konvertieren
  inet_aton (argv[1], &address.sin_addr); // Wandelt argv1 mit IP-Adresse in Adressstruktur um (in Network Byte Order)
  //inet_aton("127.0.0.1", &address.sin_addr);

  if (connect ( create_socket, (struct sockaddr *) &address, sizeof (address)) == 0)
  {
    printf ("Connection with server (%s) established\n", inet_ntoa (address.sin_addr));
    receiveMessage(create_socket,buffer);
  }
  else
  {
    perror("Connect error - no server available");
    return EXIT_FAILURE;
  }

  do {
    /* Angabe des Requests */
    printf ("Send message: ");
    sendMessage(create_socket,buffer);

    /* Server-Antwort abwarten */
    receiveMessage(create_socket,buffer);
    if(strcmp(buffer,"OK-SEND\n") == 0)
    {
      sendProtocol(create_socket,buffer);
    }
    else if(strcmp(buffer,"OK-READ\n") == 0)
    {
      readProtocol(create_socket,buffer);
    }

  }
  while (strcmp (buffer, "QUIT\n") != 0);
  close (create_socket);
  return EXIT_SUCCESS;
}
