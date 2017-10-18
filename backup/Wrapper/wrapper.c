#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
extern int bufsize; // bufsize vom Server, Client C-Programm

// gibt die gesendeten Bytes zurück
int sendLength(int socket, int value) {

	int sentBytes = 0;
	int length = htonl(value);

	if ((sentBytes = send(socket, &length, sizeof length, 0)) == -1) {
		perror("send error");
		return -1;
	}
	return sentBytes;
}

// gibt die empfangenen Bytes zurück
int recvLength(int socket, int *value) {

	int recvBytes= 0;
	int length;

	// int recv(Socket Deskriptor, Pointer auf Empfangsbuffer für Daten, Größe, Flags), blockiert bis Daten empfangen werden
	if ((recvBytes = recv(socket, &length, sizeof length, 0)) == -1) {
		perror("recv error");
		return -1;
	}
	*value = ntohl(length);
	return recvBytes;
}

int sendString(int socket, char *buffer) {

	int total = 0; // how many bytes we've sent
	int length = strlen(buffer) + 1; //string length
	int bytesleft = length; // how many we have left to send
	int n;

	sendLength(socket, length);
	//printf("Sending: %s - length %d \n", buffer, length);

	while (total < length) {

		n = send(socket, buffer + total, bytesleft, 0);
		if (n == -1) { break; }
		total += n;
		bytesleft -= n;
	}

	length = total; // return number actually sent here
	return n == -1 ? -1 : 0; // return -1 on failure, 0 on success
}


//return received bytes
int recvString(int socket, char **buffer) {

	//get length of transmission
	int length;
	recvLength(socket, &length);
	//printf("Receiving: %d bytes \n", length);

	//realloc here
	if(bufsize < length) {

		//printf("Reallocating: buffer %d < message %d\n", bufsize, length);

		//next highest power of 2
		int i = length;

		i--;
		i |= i >> 1;
		i |= i >> 2;
		i |= i >> 4;
		i |= i >> 8;
		i |= i >> 16;
		i++;

		*buffer = realloc(*buffer, i);
		bufsize = i;
		printf("reallocated: %d \n", bufsize);
	}

	int total = 0; //how many bytes we've received
	int bytesleft = length; //bytes left to receive
	int n;

	while (total < length) {

		n = recv(socket, *buffer + total, bytesleft, 0);
		if (n == -1) { break; }
		total += n;
		bytesleft -= n;
	}

	//while bytes received < length {recv}
	//return recv(socket, buffer, bufsize - 1, 0);
	return total;
}
