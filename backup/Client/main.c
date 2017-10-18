/* myclient.c */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../Wrapper/wrapper.h"

#define BUF 8
#define COLOURMODE 1 // Colourmode nur unter Linux möglich
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KWHT  "\x1B[37m"

size_t bufsize = BUF;

void receiveMessage(int socket, char *buffer)
{
	int size = recvString(socket, &buffer);

	if (size > 0)
	{
		buffer[size]= '\0';
		if(COLOURMODE) printf("%sServer: %s", KGRN, buffer);
		else printf("Server: %s", buffer);
	}
}

int main (int argc, char **argv) {
	int create_socket;
	char *buffer;
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

	// Aufbau einer Verbindung: int connect (Socket Deskriptor, Pointer auf Adressstruktur mit Daten des Servers, Größe)
	if (connect ( create_socket, (struct sockaddr *) &address, sizeof (address)) == 0)
	{
		if(COLOURMODE) printf ("%sConnection with server (%s) established\n", KRED, inet_ntoa (address.sin_addr));
		else printf ("Connection with server (%s) established\n", inet_ntoa (address.sin_addr));

		//size = recv(create_socket,buffer,BUF-1, 0);
		buffer = malloc(sizeof(char) * BUF);

		size = recvString(create_socket, &buffer); // Aufruf Methode recvString von wrapper.c

		if (size > 0)
		{
			buffer[size]= '\0';
			if(COLOURMODE) printf("%s%s%s", KGRN,buffer,KWHT);
			else printf("%s", buffer);
		}
	}
	else
	{
		perror("Connect error - no server available");
		return EXIT_FAILURE;
	}

	//char **ptrbuf = &buffer;
	do {
		//printf ("Send message: ");

		//fgets (buffer, BUF, stdin);

		//wenn der buffer nicht reicht, reallokiert getline mit realloc()
		getline(&buffer, &bufsize, stdin);
		//printf("Bufsize: %d\n", (int) bufsize);
		//send(create_socket, buffer, strlen (buffer), 0);
		sendString(create_socket, buffer); //send message, Aufruf Methode sendtring von wrapper.c

		// Falls nötig, wird in recvString reallokiert
		size = recvString(create_socket, &buffer);

		if (size > 0)
		{
			buffer[size]= '\0';
			if(COLOURMODE) printf("%sServer: %s",KGRN, buffer);
			else printf("Server: %s", buffer);
		}

		if(strcmp(buffer, "OK - SEND\n") == 0)
		{
			if(COLOURMODE) {
				printf("%s=============================================================\n",KWHT);
				printf("%sMessage: Sender \\n Receiver \\n Subject \\n Content \\n.\\n\n",KWHT);
			}
			else {
				printf("=============================================================\n");
				printf("Message: Sender \\n Receiver \\n Subject \\n Content \\n.\\n\n");
			}
			// sender
			do
			{
				receiveMessage(create_socket,buffer); // Receive Sender:
				getline(&buffer, &bufsize, stdin); //wenn der buffer nicht reicht, reallokiert getline mit realloc()
				sendString(create_socket, buffer); //send message, Aufruf Methode sendtring von wrapper.c
				receiveMessage(create_socket,buffer); // Receive OK or ERR
			}
			while(strcmp("OK\n",buffer) != 0);


			// empfänger
			do
			{
				receiveMessage(create_socket,buffer); // Receive Sender:
				getline(&buffer, &bufsize, stdin); //wenn der buffer nicht reicht, reallokiert getline mit realloc()
				sendString(create_socket, buffer); //send message, Aufruf Methode sendtring von wrapper.c
				receiveMessage(create_socket,buffer); // Receive OK or ERR
			}
			while(strcmp("OK\n",buffer) != 0);

			// subject
			// empfänger
			do
			{
				receiveMessage(create_socket,buffer); // Receive Sender:
				getline(&buffer, &bufsize, stdin); //wenn der buffer nicht reicht, reallokiert getline mit realloc()
				sendString(create_socket, buffer); //send message, Aufruf Methode sendtring von wrapper.c
				receiveMessage(create_socket,buffer); // Receive OK or ERR
			}
			while(strcmp("OK\n",buffer) != 0);

			receiveMessage(create_socket,buffer);
			// content
			while(strcmp(buffer,".\n") != 0)
			{
				getline(&buffer, &bufsize, stdin);
				sendString(create_socket, buffer); //send message, Aufruf Methode sendtring von wrapper.c
			}
			receiveMessage(create_socket,buffer);
			receiveMessage(create_socket,buffer);


			if(COLOURMODE) printf("%s=============================================================\n",KWHT);
			else printf("=============================================================\n");
			receiveMessage(create_socket,buffer);
		}

		else if(strcmp(buffer, "OK - LIST\n") == 0)
		{
			printf("=============================================================\n");
			receiveMessage(create_socket,buffer);

			getline(&buffer, &bufsize, stdin);
			sendString(create_socket, buffer); //send message, Aufruf Methode sendtring von wrapper.c

			receiveMessage(create_socket,buffer);
			if(atoi(buffer) > 0)
			{
				receiveMessage(create_socket,buffer);
			}

			printf("=============================================================\n");
			receiveMessage(create_socket,buffer);
		}
		// Read
		else if(strcmp(buffer, "OK - READ\n") == 0)
		{
			printf("=============================================================\n");
			// username
			receiveMessage(create_socket,buffer);
			getline(&buffer, &bufsize, stdin);
			sendString(create_socket, buffer);

			// nachrichtennummer
			receiveMessage(create_socket,buffer);
			getline(&buffer, &bufsize, stdin);
			sendString(create_socket, buffer);

			receiveMessage(create_socket,buffer);
			if(strcmp(buffer,"OK\n") == 0)
			{
				receiveMessage(create_socket,buffer);
			}
			printf("=============================================================\n");
			receiveMessage(create_socket,buffer);
		}
		// Delete
		else if(strcmp(buffer, "OK - DEL\n") == 0)
		{
			printf("=============================================================\n");
			// username
			receiveMessage(create_socket,buffer);
			getline(&buffer, &bufsize, stdin);
			sendString(create_socket, buffer);

			// nachrichtennummer
			receiveMessage(create_socket,buffer);
			getline(&buffer, &bufsize, stdin);
			sendString(create_socket, buffer);

			receiveMessage(create_socket,buffer);
			if(strcmp(buffer,"OK\n") == 0)
			{
				receiveMessage(create_socket,buffer);
			}
			printf("=============================================================\n");
			receiveMessage(create_socket,buffer);

		}

	}
	while (strcmp(buffer, "OK - QUIT\n") != 0);

	free(buffer); // Speicher freigeben
	close (create_socket);
	return EXIT_SUCCESS;
}
