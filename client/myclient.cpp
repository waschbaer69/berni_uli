// myclient.cpp

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h> /* O_WRONLY, O_CREAT */
#include <sys/stat.h>

#include <iostream>
#include <sstream>
#include <sys/sendfile.h>
#include <pthread.h>

#define BUF 1024

using namespace std;

// Sends a message to the client
void send_message(int socket, char* send_buffer, string message)
{
  strcpy(send_buffer,message.c_str());
  //printf("strlen: %d",(int)strlen(send_buffer));
  send(socket, send_buffer, strlen(send_buffer),0);
}

// receives a message from the client
int receive_message(int socket, char* recv_buffer)
{
  int size_msg = recv (socket, recv_buffer, BUF-1, 0);
  if(size_msg > 0) recv_buffer[size_msg] = '\0'; // append 0 Byte, if no problems
  return size_msg;
}

void send_file(int sock, char *file_name)
{
  int fd;
  int send_error = 0;
  int sent_bytes = 0;
  struct stat file_stat;
  char file_size[BUF];
  off_t offset;
  int remain_data;
  ssize_t len;

  fd = open(file_name, O_RDONLY);
  if (fd == -1)
  {
    fprintf(stderr, "Error opening file --> %s\n", strerror(errno));

    //exit(EXIT_FAILURE);
    send_error = 1;
  }

  if(send_error != 1)
  {
    /* Get file stats */
    if (fstat(fd, &file_stat) < 0)
    {
      fprintf(stderr, "Error fstat --> %s\n", strerror(errno));

      //exit(EXIT_FAILURE);
      send_error = 1;
    }

    fprintf(stdout, "File Size: \n%d bytes\n", (int) file_stat.st_size);

    sprintf(file_size, "%d", (int) file_stat.st_size);
  }
  else
  {
    if(strlen(file_name) == 0)
    {
      sprintf(file_size, "%d", (int) 0);
    }
    else
    {
      sprintf(file_size, "%d", (int) -1);
    }

    fprintf(stdout, "File Size: \n%s bytes\n", file_size);
  }

  /* Sending file size */
  len = send(sock, file_size, sizeof(file_size), 0);
  if (len < 0)
  {
    fprintf(stderr, "Error on sending greetings --> %s", strerror(errno));

    //exit(EXIT_FAILURE);
    send_error = 1;
  }

  offset = 0;
  remain_data = file_stat.st_size;
  /* Sending file data, if no error occurred */
  if(send_error != 1 && file_size>0)
  {
    while (((sent_bytes = sendfile(sock, fd, &offset,BUF)) > 0) && (remain_data > 0))
    {
      //fprintf(stdout, "1. Server sent %d bytes from file's data, offset is now : %d and remaining data = %d\n", sent_bytes, (int) offset, remain_data);
      remain_data -= sent_bytes;
      //fprintf(stdout, "2. Server sent %d bytes from file's data, offset is now : %d and remaining data = %d\n", sent_bytes, (int) offset, remain_data);
    }
  }
}

int main (int argc, char **argv) {
  int server_socket_fd;
  char buffer[BUF];
  struct sockaddr_in address;
  int isLoggedIn = 0;
  string user_id;
  int size;

  //check if necessary arguments are there
  if( argc != 3 ){
    printf("Usage: %s ServerAdresse Port\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  if ((server_socket_fd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
  {
    perror("Socket error");
    return EXIT_FAILURE;
  }

  //open connection to the server
  memset(&address,0,sizeof(address));
  address.sin_family = AF_INET;
  address.sin_port = htons (atoi(argv[2]));
  inet_aton (argv[1], &address.sin_addr);

  if (connect ( server_socket_fd, (struct sockaddr *) &address, sizeof (address)) == 0)
  {
    printf ("Connection with server (%s:%d) established\n", inet_ntoa (address.sin_addr), ntohs(address.sin_port));

    size = receive_message(server_socket_fd,buffer);
    if (size>0)
    {
      printf("%s",buffer);
    }
  }
  else
  {
    perror("Connect error - no server available");
    return EXIT_FAILURE;
  }


  while(1){
    //scan the console for a request and send it to the server
    //requests are generated as c++strings, then converted to char arrays and send to the server
    //the server receeives the content of the char array and deals with it accordingly
    char response[BUF];
    cin.clear();
    cout.clear();
    string command;
    getline(cin,command);

    if(command == "LOGIN")
    {
      string str_request, uid, pwd;
      char request[1024];
      printf("UID: ");
      getline(cin,uid);
      char* password = getpass("Password: "); //do not show input
      pwd = password; //convert to string
      str_request = "LOGIN\n"+uid+"\n"+pwd+"\n";
      send_message(server_socket_fd,request,str_request.c_str());

      /* get OK or ERR back */
      receive_message(server_socket_fd,response);
      if(strcmp(response,"OK\n") == 0)
      {
        isLoggedIn = 1;
        user_id = uid;
      }
      printf("%s",response);
    }
    else if((command == "SEND") && (isLoggedIn == 1)){
      //read the message
      string string_request,sender,rec,subject,content,attachement_str,str;
      string_request = "";
      content="";
      char request[1024];
      //printf("Sender: ");
      //getline(cin,sender);
      printf("Receiver: ");
      getline(cin,rec);
      printf("Subject: ");
      getline(cin,subject);
      printf("Content: ");
      while(1){
        getline(cin,str);
        if(str == ".") break;
        content=content+str+"\n";
      }
      printf("Path to Attachement (optional): ");
      getline(cin,attachement_str);
      string_request = "SEND\n"+user_id+"\n"+rec+"\n"+subject+"\n"+content+".\n"+attachement_str;

      //send the message to the server
      //strcpy(request, string_request.c_str());
      //send(server_socket_fd, request, strlen(request), 0);
      send_message(server_socket_fd,request,string_request.c_str());

      char attachement[BUF];
      strcpy(attachement,attachement_str.c_str());
      /* send file here */
      send_file(server_socket_fd,attachement);

      //get OK or ERR back
      //size=recv(server_socket_fd,response,BUF-1, 0);
      //response[size]= '\0';
      receive_message(server_socket_fd,response);
      printf("%s",response);
    }
    else if ((command == "LIST") && (isLoggedIn == 1)){

      //read the username
      string string_request;
      char request[1024];
      //printf("Username: ");
      //getline(cin,string_request);
      string_request = "LIST\n"+user_id;

      //send username to the server
      //strcpy(request, string_request.c_str());
      //send(server_socket_fd, request, strlen(request), 0);
      send_message(server_socket_fd,request,string_request.c_str());

      //get the list back
      //size=recv(server_socket_fd,response,BUF-1, 0);
      //response[size]= '\0';
      receive_message(server_socket_fd,response);
      printf("%s",response);

    }
    else if((command == "READ") && (isLoggedIn == 1)){

      //read the username and number
      string string_request,num;
      char request[1024];
      //printf("Username: ");
      //getline(cin,string_request);
      printf("Message-Number: ");
      getline(cin,num);
      string_request = "READ\n"+user_id+"\n"+num;

      //send username to the server
      //strcpy(request, string_request.c_str());
      //send(server_socket_fd, request, strlen(request), 0);
      send_message(server_socket_fd,request,string_request.c_str());

      //get the message back
      //size=recv(server_socket_fd,response,BUF-1, 0);
      //response[size]= '\0';
      receive_message(server_socket_fd,response);
      printf("%s",response);

    }
    else if((command == "DEL") && (isLoggedIn == 1)){

      //read the username and number
      string string_request,num;
      char request[1024];


      //printf("Username: ");
      //getline(cin,string_request);
      printf("Message-Number: ");
      getline(cin,num);
      string_request = "DEL\n"+user_id+"\n"+num;

      //send username to the server
      //strcpy(request, string_request.c_str());
      //send(server_socket_fd, request, strlen(request), 0);
      send_message(server_socket_fd,request,string_request.c_str());

      //get the message back
      //size=recv(server_socket_fd,response,BUF-1, 0);
      //response[size]= '\0';
      receive_message(server_socket_fd,response);
      printf("%s",response);

    }
    else if(command == "QUIT"){

      //send a QUIT command, so that the server knows the client has disconnected
      string string_request = "QUIT";
      char request[1024];
      //strcpy(request, string_request.c_str());
      //send(server_socket_fd, request, strlen(request), 0);
      send_message(server_socket_fd,request,string_request.c_str());

      break;

    }
    else {

      cout << "Invalid Input, you might have to login to use SEND,LIST,DEL and READ.\n";

    }

  }

  close (server_socket_fd);
  return EXIT_SUCCESS;
}
