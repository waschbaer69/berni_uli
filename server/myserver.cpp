/* myserver.c */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <iostream>
#include <fstream>
#include <chrono>

#include "server.h"
#define BUF 1024

struct arg_struct {
    int socket;
    struct sockaddr_in addr;
    char* directory;
};

void * threading_socket (void *arguments)
{
  struct arg_struct *args = (struct arg_struct *)arguments;

  //int clientfd = *((int *)arg);
  pthread_detach (pthread_self ());

  //starts the server, initializes object from class server.cpp
  server *c = new server(args->socket, args->addr, args->directory);
  //waiting for requests
  c->wait_for_request();
  delete c;
  return NULL;
}

int main (int argc, char **argv) {

  if( argc != 3 ){
    printf("Usage: %s Port Folderpath\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  int server_socket, client_socket;;
  socklen_t addrlen;
  char buffer[BUF];
  struct sockaddr_in address, cliaddress;

  //open the socket for a client to connect
  server_socket = socket (AF_INET, SOCK_STREAM, 0);

  memset(&address,0,sizeof(address));
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons (atoi(argv[1]));

  if (bind ( server_socket, (struct sockaddr *) &address, sizeof (address)) != 0) {
    perror("bind error");
    return EXIT_FAILURE;
  }
  listen (server_socket, 5);

  addrlen = sizeof (struct sockaddr_in);

  while (1) {

    //listen until a client connects to the socket
    printf("Waiting for connections...\n");
    client_socket = accept ( server_socket, (struct sockaddr *) &cliaddress, &addrlen );

    if (client_socket > 0){
      printf ("Client connected from %s:%d...\n", inet_ntoa (cliaddress.sin_addr),ntohs(cliaddress.sin_port));

      //check if banned or not
      string filepath(argv[2]);
      ifstream inFile(filepath+"/blacklist");

      ofstream outFile(filepath+"/blacklist_temp");

      //check if exists
      if(!inFile) {
        perror("Couldn't open blacklist");
        exit(EXIT_FAILURE);
      }

      //current time
      unsigned long now = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
      unsigned long timestamp;
      string ip;
      int banned = 0;

      //read until no more lines
      while(inFile >> ip) {
        inFile >> timestamp;
        cout << ip << " " << to_string(timestamp) << endl;

        if(strcmp(ip.c_str() , inet_ntoa(cliaddress.sin_addr)) == 0) {
          //if connection is in there check timestamp
          if(timestamp > now) {
            //if banned
            outFile << ip << " " << timestamp << endl;
            banned = 1;
          } else {
            //remove line from file (don't write to new file)
            //unbanned
            cout << "Unbanning " << ip << endl;
            banned = 0;
          }
        } else {
          outFile << ip << " " << timestamp << endl;
        }
      }

      inFile.close();
      outFile.close();

      remove( (filepath+"/blacklist").c_str() );
      rename( (filepath+"/blacklist_temp").c_str(), (filepath+"/blacklist").c_str() );

      if(banned == 0) {
        //send the client a welcome message
        strcpy(buffer,"Welcome to myserver, Please enter your command:\n");
        send(client_socket, buffer, strlen(buffer),0);

        pthread_t th;

        struct arg_struct args;
        args.socket = client_socket;
        args.addr = cliaddress;
        args.directory = argv[2];

        pthread_create(&th,NULL,threading_socket,(void *)&args);
      } else {
        //send the client a piss off message
        strcpy(buffer,"You are still IP banned\n");
        send(client_socket, buffer, strlen(buffer),0);
        close(client_socket);
      }
    }
  }
  close(server_socket);
  return EXIT_SUCCESS;
}
