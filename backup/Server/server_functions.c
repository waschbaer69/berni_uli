#include <sys/types.h>
#include <sys/socket.h> // socket (), bind ()
#include <netinet/in.h> // struct sockaddr_in
#include <arpa/inet.h> // inet_ntoa()
#include <unistd.h> // read(), write(), close()
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "../Wrapper/wrapper.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <ctype.h>

#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KWHT  "\x1B[37m"
#define COLOURMODE 1 // Colourmode nur unter Linux möglich

void sendMessage(int socket, char* buffer, char* text)
{
	strcpy(buffer, text);
	//implement command sequence
	sendString(socket, buffer);
	if(COLOURMODE) printf("%s%s", KGRN,buffer);
	else printf("%s", buffer);
}


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

char * append_strings(const char * old, const char * new, const char * last)
{
	size_t len = strlen(old) + strlen(new) + strlen(last)+ 1; // get size of new string
	char *out = malloc(len); // allocate a pointer to the new string
	sprintf(out, "%s%s%s", old, new, last); // concat both strings and return
	return out;
}

int createFile(char* argument, char* username, char* string)
{
	char* directory_path = append_strings(argument,"/",username);
	int error = 0;

	struct stat st = {0};

	// Create dir if it does not exist
	if (stat(directory_path, &st) == -1) {
		mkdir(directory_path, 0700);
	}

	DIR *b;
	struct dirent *dir2;
	b = opendir(directory_path);

	// create file with calculated number and write string into it
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
		char * new = append_strings(directory_path,str,".txt");

		fp = fopen(new, "w+");
		fputs(string, fp);
		fclose(fp);

		free(new);
		closedir(b);
	}
	else{
		error = 1;
	}
	free(directory_path);

	return error;
}

void getSubject(char* argument, char* username, int socket, char *buffer)
{
	char* directory_path = append_strings(argument,"/",username);
	printf("%s\n",directory_path);

	char* filepath;
	filepath = malloc(strlen(directory_path)+1);

	char subject[80 + 1]; //80 zeichen + \0
	char* listing = malloc(1);

	// ==================================================================
	struct dirent **namelist;

	int n = scandir(directory_path, &namelist, NULL, alphasort); //sortiere alphabetisch

	printf("%i\n", n-2); //anzahl elemente ohne . und ..

	if(n < 0) { //fehler wenn ordner nicht existiert
		sendMessage(socket, buffer, "ERR\n");
	} else {
		for(int i = 2; i < n; i++) { //ersten 2 elemente überspringen ( . und .. )

			filepath = append_strings(directory_path,"/",namelist[i]->d_name); //filenamen anhängen
			FILE *fd = fopen(filepath, "r");    //read-only

			for(int i = 0; i < 3; i++) { //wir wollen die 3. zeile (subject)
				fgets(subject, sizeof(subject), fd);
			}

			listing = append_strings(listing,"",subject);
			fclose(fd);

			free(namelist[i]);
		}
		free(namelist);
	}

	char str[13]; //All numbers that are representable by int will fit in a 12-char-array without overflow
	sprintf(str, "%d\n", n);

	sendMessage(socket, buffer, str);
	if(n > 0)
	{
		sendMessage(socket, buffer, listing);
	}

	free(listing);
	free(filepath);
	free(directory_path);

}

void getMessage(char* argument, char* username, int number, int socket, char *buffer)
{
	char* directory_path = append_strings(argument,"/",username);
	char* string = (char*)malloc(1);

	printf("%s\n",directory_path);

	char* filepath;
	filepath = malloc(strlen(directory_path)+1);

	struct dirent **namelist;

	int n = scandir(directory_path, &namelist, NULL, alphasort); //sortiere alphabetisch

	printf("%i\n", n-2); //anzahl elemente ohne . und ..

	if(n < 0 || number <= 0 || number > (n-2)) { //fehler wenn ordner nicht existiert oder nummer negativ oder nummer größer als anzahl nachrichten
		sendMessage(socket, buffer, "ERR\n");
	} else {
		for(int i = 2; i < n; i++) { //ersten 2 elemente überspringen ( . und .. )

			// Aktuelle Nachrichtennummer entspricht gewünschter Nummer
			if((i-1) == number) {
				filepath = append_strings(directory_path,"/",namelist[i]->d_name); //filenamen anhängen
				FILE *f;
				char c;
				f=fopen(filepath,"rt");

				while((c=fgetc(f))!=EOF){
					char str[2];
					str[0] = c;
					str[1] = '\0';
					string = append_strings(string,"",str);
				}
				fclose(f);
			}
			free(namelist[i]);
		}
		free(namelist);
	}
	printf("%s",string);
	//sendMessage(socket, buffer, string);
	free(string);
	free(filepath);
	free(directory_path);
}


void deleteMessage(char* argument, char* username, int number, int socket, char *buffer)
{
	char* directory_path = append_strings(argument,"/",username);
	char *source; // String für Inhalt

	printf("%s\n",directory_path);

	char* filepath;
	filepath = malloc(strlen(directory_path)+1);
	struct dirent **namelist;

	int n = scandir(directory_path, &namelist, NULL, alphasort); //sortiere alphabetisch

	printf("%i\n", n-2); //anzahl elemente ohne . und ..

	if(n < 0 || number <= 0 || number > (n-2)) { //fehler wenn ordner nicht existiert oder nummer negativ oder nummer größer als anzahl nachrichten
		sendMessage(socket, buffer, "ERR\n");
	} else {
		for(int i = 2; i < n; i++) { //ersten 2 elemente überspringen ( . und .. )

			// Aktuelle Nachrichtennummer entspricht gewünschter Nummer
			if((i-1) == number) {
				filepath = append_strings(directory_path,"/",namelist[i]->d_name); //filenamen anhängen
				printf("Removing file: %s\n", namelist[i]->d_name);
				printf("filepath: %s\n", filepath);
				if (remove(filepath) == 0) {
					//success
					sendMessage(socket, buffer, "OK\n");
				} else {
					//error
					sendMessage(socket, buffer, "ERR\n");
				}
			}
			free(namelist[i]);
		}
		free(namelist);
	}

	free(filepath);
	free(directory_path);
}
