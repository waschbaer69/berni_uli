/* myserver.c */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define BUF 1024


/* Überprüft, ob der String die richtige Länge hat.
  Gibt 0 zurück, falls dies der Fall ist */
int checkLength(char* buffer, int max_length)
{
	int size = strlen(buffer)-1;
	int error = 0;
	if(size <= max_length)
	{
		error = 0;
	}
	else
	{
		error = 1;
	}
	return error;
}

/* Sendet String "text" an Client */
void sendMessage(int socket, char* buffer, char* text)
{
  strcpy(buffer,text);
  send(socket, buffer, strlen(buffer),0);
	printf("%s",buffer);
}

/* Empfängt String vom Client und speichert in Buffer */
void receiveMessage(int socket, char* buffer)
{
  int size=recv(socket,buffer,BUF-1, 0);
  if (size>0)
  {
    buffer[size]= '\0';
    printf("%s",buffer);
  }
}

/* Schaut ob der Input des Client die richtige Größe hat und sendet in diesem Fall "OK"*/
void getAndCheckClientInput(char* string, int socket, char* buffer, int maxsize)
{
	do
	{
		receiveMessage(socket,buffer);
		if(checkLength(buffer,maxsize) != 0) sendMessage(socket, buffer, "ERR\n");
		else
		{
			string = strcpy(string,buffer);
			string = strcat(string,"\n");
			sendMessage(socket, buffer, "OK\n");
		}
	}
	while(strcmp(buffer,"OK\n")!=0);
}

/* Protokoll für SEND-Modus mit Error-Prevention */
void sendProtocol(int socket, char *buffer, char* directory)
{
	char sender[9]; char receiver[9]; char subject[81];

	printf("SEND:\n");
	printf("=============================================================\n");
  sendMessage(socket,buffer,"OK-SEND\n");

	getAndCheckClientInput(sender,socket,buffer,8); // Sender
	getAndCheckClientInput(receiver,socket,buffer,8); // Receiver
	getAndCheckClientInput(subject,socket,buffer,80); // Subject

	/* Für den Inhalt wird solange eine Nachricht empfangen bis diese nurmehr ein Punkt ist.
		 Außerdem wird ein dynamischer String erstellt in dem der Inhalt gespeichert wird */
	char* content;
	int count = 0;
	while(strcmp(buffer,".\n") != 0)
	{
		receiveMessage(socket,buffer);
		if(count == 0){
			content = malloc(strlen(buffer)+1);
			content = strcpy(content,buffer);
		}
		else{
			/* Zwei Strings aneinanderhängen */
			content = realloc(content,strlen(content) + strlen(buffer) + 1);
			content = strcat(content,buffer);
		}
		count++;
	};


	char * all;
	all = malloc(strlen(sender)+strlen(content)+1);
	all = strncpy(all,sender,strlen(sender)-1);
	all = strcat(all,content);

	printf("content: %s", all);

	free(content);
	free(all);
	printf("=============================================================\n");
}

int main (int argc, char **argv) {
  int create_socket, new_socket;
  socklen_t addrlen;
  char buffer[BUF];
  int size;
  struct sockaddr_in address, cliaddress;

  if( argc < 3 ){
    printf("Usage: %s Port directory_path\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  create_socket = socket (AF_INET, SOCK_STREAM, 0);

  memset(&address,0,sizeof(address));
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons (atoi(argv[1])); // Gebenen Port(argv[1]) in integer (atoi) und host-to-network für short konvertieren

  if (bind ( create_socket, (struct sockaddr *) &address, sizeof (address)) != 0) {
    perror("bind error");
    return EXIT_FAILURE;
  }
  listen (create_socket, 5);

  addrlen = sizeof (struct sockaddr_in);

  while (1) {
    printf("Waiting for connections...\n");
    new_socket = accept ( create_socket, (struct sockaddr *) &cliaddress, &addrlen );
    if (new_socket > 0)
    {
      printf ("Client connected from %s:%d...\n", inet_ntoa (cliaddress.sin_addr),ntohs(cliaddress.sin_port));
      strcpy(buffer,"Welcome to myserver, Please enter your command:\n");
      send(new_socket, buffer, strlen(buffer),0);
    }
    do {

      /* Client gibt Request ein */
      size = recv (new_socket, buffer, BUF-1, 0);
      if( size > 0)
      {
        buffer[size] = '\0';
        printf ("Message received: %s\n", buffer);
        /* Requests erkennen */
        if (strncmp(buffer, "SEND", 4) == 0) {
          sendProtocol(new_socket,buffer,argv[2]); // call function
        }
      }
      else if (size == 0)
      {
        printf("Client closed remote socket\n");
        break;
      }
      else
      {
        perror("recv error");
        return EXIT_FAILURE;
      }
    } while (strncmp (buffer, "QUIT", 4)  != 0);
    close (new_socket);
  }
  close (create_socket);
  return EXIT_SUCCESS;
}
