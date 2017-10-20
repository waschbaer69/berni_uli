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

#include <sys/stat.h>
#include <dirent.h>
#include <ctype.h>

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

/*
Speichert File im Directory
*/
int createFile(char* argument, char* username, char* string)
{
	int error = 0;
	char directory_path[256];
	strcpy(directory_path,argument);
	strcat(directory_path,"/");
	strncat(directory_path,username,9);

	struct stat st = {0};

	// Direcotry erstellen falls es nicht existiert
	if (stat(directory_path, &st) == -1) {
		mkdir(directory_path, 0700);
	}

	DIR *b;
	struct dirent *dir2;
	b = opendir(directory_path);

	// File mit der kalkulierten Number öffnen und String hineinschreiben
	if (b)
	{
		int maxnumber = 0;
		while ((dir2 = readdir(b)) != NULL)
		{
			int filenumber = 0;
			filenumber = atoi(dir2->d_name);
			if(filenumber != 0)
			{
				printf("%s\n", dir2->d_name);
			}
			if(filenumber >= maxnumber) maxnumber = filenumber;
		}

		FILE *fp;
		char str[13]; // mit 12 kann man laut Internet jede Zahl darstellen, wegen "/" also 13
		sprintf(str, "/%d",(maxnumber+1));

		char file[256];
		strcpy(file,directory_path);
		strcat(file,str);
		strcat(file,".txt");

		fp = fopen(file, "w+");
		fputs(string, fp);
		fclose(fp);

		closedir(b);
	}
	else{
		error = 1;
	}
	return error;
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

	if(createFile(directory,receiver,all) == 0) sendMessage(socket,buffer,"Save-File-OK\n");
	else sendMessage(socket,buffer,"Save-File-ERR\n");;
	free(content);
	free(all);
	printf("=============================================================\n");
}

void getMessage(char* argument, char* username, int number, int socket, char *buffer)
{
	char directory_path[256];
	char filepath[256];
	strcpy(directory_path,argument);
	strcat(directory_path,"/");
	strncat(directory_path,username,9);

	struct dirent **namelist;

	int n = scandir(directory_path, &namelist, NULL, alphasort); //sortiere alphabetisch
	//printf("%i\n", n-2); //anzahl elemente ohne . und ..

	if(n < 0 || number <= 0 || number > (n-2)) { //fehler wenn ordner nicht existiert oder nummer negativ oder nummer größer als anzahl nachrichten
		sendMessage(socket, buffer, "ERR\n");
	} else {
		for(int i = 2; i < n; i++) { //ersten 2 elemente überspringen ( . und .. )

			// Aktuelle Nachrichtennummer entspricht gewünschter Nummer
			if((i-1) == number) {

				strcpy(filepath,directory_path);
				strcat(filepath,"/");
				strcat(filepath,namelist[i]->d_name);

				char *source = NULL;
				FILE *fp = fopen(filepath, "r");
				if (fp != NULL) {
					/* Go to the end of the file. */
					if (fseek(fp, 0L, SEEK_END) == 0) {
						/* Get the size of the file. */
						long bufsize = ftell(fp);
						if (bufsize == -1) { /* Error */ }

						/* Allocate our buffer to that size. */
						source = malloc(sizeof(char) * (bufsize + 1));

						/* Go back to the start of the file. */
						if (fseek(fp, 0L, SEEK_SET) != 0) { /* Handle error here */ }

						/* Read the entire file into memory. */
						size_t newLen = fread(source, sizeof(char), bufsize, fp);
						if (newLen == 0) {
							fputs("Error reading file", stderr);
						} else {
							source[++newLen] = '\0'; /* Just to be safe. */
						}
					}
					sendMessage(socket, buffer, source);
					fclose(fp);
				}

				free(source); /* Don't forget to call free() later! */
			}
			free(namelist[i]);
		}
		free(namelist);
	}

}

/* Protokoll für READ-Modus mit Error-Prevention */
void readProtocol(int socket, char *buffer, char* directory)
{
	char receiver[10];
	printf("READ:\n");
	printf("=============================================================\n");
	sendMessage(socket,buffer,"OK-READ\n");
	getAndCheckClientInput(receiver,socket,buffer,8); // Receiver
	receiveMessage(socket,buffer);
	int n = atoi(buffer);
	getMessage(directory,receiver,n,socket,buffer);
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
				else if (strncmp(buffer, "READ", 4) == 0) {
					readProtocol(new_socket,buffer,argv[2]); // call function
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
