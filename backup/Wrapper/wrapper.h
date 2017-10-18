#ifndef WRAPPER_H_INCLUDED
#define WRAPPER_H_INCLUDED

int sendString(int socket, char *buffer);
int recvString(int socket, char **buffer);

int sendLength(int socket, int length);
int recvLength(int socket, int length);
#endif // WRAPPER_H_INCLUDED
