#ifndef SERVER_H_INCLUDED
#define SERVER_H_INCLUDED

void sendMessage(int socket, char* buffer, char* text);
int checkLength(char* buffer, int max_length);
char * append_strings(const char * old, const char * new, const char * last);
int createFile(char* argument, char* username, char* string);
void getSubject(char* argument, char* username, int socket, char *buffer);
void getMessage(char* argument, char* username, int number, int socket, char *buffer);
void deleteMessage(char* argument, char* username, int number, int socket, char *buffer);

#endif // WRAPPER_H_INCLUDED
