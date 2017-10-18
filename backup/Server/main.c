/* myserver.c */
#include <sys/types.h>
#include <sys/socket.h> // socket (), bind ()
#include <netinet/in.h> // struct sockaddr_in
#include <arpa/inet.h> // inet_ntoa()
#include <unistd.h> // read(), write(), close()
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../Wrapper/wrapper.h"
#include "./server_functions.h"
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <ctype.h>


#define MAXLINE 1500

#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KWHT  "\x1B[37m"
#define COLOURMODE 1 // Colourmode nur unter Linux möglich

#define BUF 64
int bufsize = BUF;

void receiveMessage(int socket, char *buffer)
{
	// Falls nötig, wird in recvString reallokiert
	int size = recvString(socket, &buffer);

	if (size > 0)
	{
		buffer[size]= '\0';
		if(COLOURMODE) printf("%sClient: %s",KRED, buffer);
		else printf("Client: %s", buffer);
	}
}



void sendProtocol(int socket, char *buffer, char* argument)
{
	char* sender;
	char* receiver;
	char* subject;
	char* content =  (char*) malloc(1);;

	if(COLOURMODE) printf("%s=============================================================\n",KWHT);
	else printf("=============================================================\n");
	// get sender
	sendMessage(socket, buffer, "OK - SEND\n");

	do
	{
		sendMessage(socket, buffer, "Sender: ");
		receiveMessage(socket,buffer);
		if(checkLength(buffer,8) != 0) sendMessage(socket, buffer, "ERR\n");
		else
		{
			sender = (char*) malloc(strlen(buffer));
			strcpy(sender,buffer);
			sendMessage(socket, buffer, "OK\n");
		}
	}
	while(strcmp(buffer,"OK\n")!=0);

	// get receiver
	do
	{
		sendMessage(socket, buffer, "Receiver: ");
		receiveMessage(socket,buffer);
		if(checkLength(buffer,8) != 0) sendMessage(socket, buffer, "ERR\n");
		else {
			receiver = (char*) malloc(strlen(buffer));
			strcpy(receiver,buffer);
			sendMessage(socket, buffer, "OK\n");
		}
	}
	while(strcmp(buffer,"OK\n")!=0);

	// get receiver
	do
	{
		sendMessage(socket, buffer, "Subject: ");
		receiveMessage(socket,buffer);
		if(checkLength(buffer,50) != 0) sendMessage(socket, buffer, "ERR\n");
		else {
			subject = (char*) malloc(strlen(buffer));
			strcpy(subject,buffer);
			sendMessage(socket, buffer, "OK\n");
		}

	}
	while(strcmp(buffer,"OK\n")!=0);

	// get Content
	sendMessage(socket, buffer, "Content: (end with .\\n) ");
	while(strcmp(buffer,".\n") != 0)
	{
		receiveMessage(socket,buffer);

		size_t len = strlen(content) + strlen(buffer)+ 1; // get size of new string
		content = (char*) realloc(content,len);
		sprintf(content, "%s%s", content, buffer); // concat both strings and return
	}

	sendMessage(socket, buffer, "OK\n");

	size_t len = strlen(sender) + strlen(receiver) + strlen(subject) + strlen(content) + 1; // get size of new string
	char * all = malloc(len); // allocate a pointer to the new string
	sprintf(all, "%s%s%s%s", sender, receiver, subject, content); // concat both strings and return

	printf("%s",all);
	if(createFile(argument,receiver,all) == 0) sendMessage(socket, buffer, "Saving Mail: OK\n");
	else {
		sendMessage(socket, buffer, "Saving Mail: ERR\n");
	}

	free(sender);
	free(receiver);
	free(subject);
	free(content);
	free(all);

	if(COLOURMODE) printf("%s=============================================================\n",KWHT);
	else printf("=============================================================\n");
}


void listProtocol(int socket, char *buffer, char* argument){
	sendMessage(socket, buffer, "OK - LIST\n");
	sendMessage(socket, buffer, "Username: ");
	receiveMessage(socket,buffer); // receive Username
	if(checkLength(buffer,8) == 0)
	{
		getSubject(argument,buffer, socket, buffer);
	}
	else{
		sendMessage(socket, buffer, "0\n");
	}
}



void readProtocol(int socket, char *buffer, char* argument){
	char* username;
	sendMessage(socket, buffer, "OK - READ\n");

	sendMessage(socket, buffer, "Username: ");
	receiveMessage(socket,buffer); // receive Username
	username = (char*) malloc(strlen(buffer));
	strcpy(username,buffer);

	sendMessage(socket, buffer, "Message-Number: ");
	receiveMessage(socket,buffer); // receive Username
	int number = atoi(buffer);

	if(checkLength(username,8) == 0 && number > 0)
	{
		sendMessage(socket, buffer, "OK\n");
		getMessage(argument, username, number, socket, buffer);
	}
	else{
		sendMessage(socket, buffer, "ERR\n");
	}
	free(username);
}

void delProtocol(int socket, char *buffer, char* argument){
	char* username;
	sendMessage(socket, buffer, "OK - DEL\n");

	sendMessage(socket, buffer, "Username: ");
	receiveMessage(socket,buffer); // receive Username
	username = (char*) malloc(strlen(buffer));
	strcpy(username,buffer);

	sendMessage(socket, buffer, "Message-Number: ");
	receiveMessage(socket,buffer); // receive Username
	int number = atoi(buffer);

	if(checkLength(username,8) == 0 && number > 0)
	{
		sendMessage(socket, buffer, "OK\n");
		deleteMessage(argument, username, number, socket, buffer);
	}
	else{
		sendMessage(socket, buffer, "ERR\n");
	}
	free(username);
}

int main (int argc, char **argv) {
	int create_socket, new_socket;
	socklen_t addrlen;
	char *buffer;
	int size;
	struct sockaddr_in address, cliaddress; // 32-Bit IPv4-Adresse

	if( argc < 3 ){
		printf("Usage: %s Port directory_path\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	create_socket = socket (AF_INET, SOCK_STREAM, 0); // Anlegen Socket: AF_INET für IPv4, SOCK_STREAM für Stream Sockets(TCP)

	memset(&address, 0, sizeof(address)); // Adressstruktur mit 0 initialisieren

	address.sin_family = AF_INET; // IPv4
	address.sin_addr.s_addr = htonl(INADDR_ANY); // eigene IP-Adresse
	address.sin_port = htons (atoi(argv[1])); // Gebenen Port(argv[1]) in integer (atoi) und host-to-network für short konvertieren

	// Zuweisen der Adresse:
	// int bind (Socket Deskriptor, Pointer auf Adressstruktur, Größe der Adressstruktur)
	if (bind ( create_socket, (struct sockaddr *) &address, sizeof (address)) != 0) {
		perror("bind error");
		return EXIT_FAILURE;
	}

	// warten auf Verbindungsanforderung: int listen(Socket Deskriptor, backlog)
	listen (create_socket, 5);

	addrlen = sizeof (struct sockaddr_in);

	listen:
	while (1) {
		if(COLOURMODE) printf("%sWaiting for connections....\n",KGRN);
		else printf("Waiting for connections....\n");

		//multithreading/proces-fork here

		// Akzeptieren einer Verbindung: accept(Socket Deskriptor, Pointer auf Adressstruktur des Clients, Größe)
		// liefert neuen Socket Deskriptor, blockiert bis Verbindung aufgebaut
		new_socket = accept ( create_socket, (struct sockaddr *) &cliaddress, &addrlen );

		if (new_socket > 0) {
			if(COLOURMODE) printf ("%sClient connected from %s:%d...\n",KGRN, inet_ntoa (cliaddress.sin_addr), ntohs(cliaddress.sin_port));
			else printf ("Client connected from %s:%d...\n",inet_ntoa (cliaddress.sin_addr), ntohs(cliaddress.sin_port));

			buffer = malloc(sizeof(char) * BUF);
			//printf("%d", (int) (sizeof(buffer)/sizeof(buffer[0])));

			//printf("%s", buffer);
			//strcpy(buffer, "Welcome to myserver, enter your Request: \n");

			//send(new_socket, buffer, strlen(buffer), 0);
			//printf("%s", buffer);


			//sendString(new_socket, buffer); // Aufrufen Methode sendString von wrapper.h
		}

		//send receive logic
		do {
			//receive msg, blocks until client send
			//size = recv (new_socket, buffer, BUF-1, 0);
			sendMessage(new_socket,buffer,"Welcome to myserver, enter your Request: \n");
			size = recvString(new_socket, &buffer); // Aufrufen Methode recvString von wrapper.h

			if(size > 0) {

				// anhängen von \0 ans Ende der Message
				buffer[size] = '\0';
				if(COLOURMODE) printf ("%sClient: %s\n", KRED, buffer);
				else printf ("Client: %s\n", buffer);

				// Requests erkennen:

				if (strncmp(buffer, "SEND", 4) == 0) {
					sendProtocol(new_socket,buffer, argv[2]);
				}

				else if (strncmp(buffer, "LIST", 4) == 0) {
					listProtocol(new_socket,buffer, argv[2]);
				}

				else if (strncmp(buffer, "READ", 4) == 0) {
					readProtocol(new_socket,buffer, argv[2]);
				}

				else if (strncmp(buffer, "DEL", 3) == 0) {
					delProtocol(new_socket,buffer, argv[2]);
				}

				else if (strncmp(buffer, "QUIT", 4) == 0) {
					strcpy(buffer, "OK - QUIT\n");
					sendString(new_socket, buffer);
					close(new_socket);
					goto listen;
				}
				else {
					strcpy(buffer, "Undefined Request \nWelcome to myserver, enter your Request: \n");
					sendString(new_socket, buffer);
				}

			}

			else if (size == 0) {
				if(COLOURMODE) printf("%sClient closed remote socket\n",KGRN);
				else printf("Client closed remote socket\n");
				//break;
				goto listen;
			}

			else {
				perror("recv error");
				//return EXIT_FAILURE;
				goto listen;
			}
		} while (strncmp (buffer, "quit", 4)  != 0);
		close (new_socket);
	}
	free(buffer); // Speicher freigeben
	close (create_socket);
	return EXIT_SUCCESS;
}
