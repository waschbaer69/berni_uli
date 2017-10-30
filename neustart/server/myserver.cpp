/* myserver.c */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "server.h"

int main (int argc, char **argv) {

    if( argc != 3 ){
        printf("Usage: %s Port Folderpath\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    //starts the server, initializes object from class server.cpp
    server *c = new server(atoi(argv[1]), argv[2]);

    while (1) {
        //wait for a connection from a client
        c->wait_for_connection();
    }
    delete c;
    return EXIT_SUCCESS;
}
