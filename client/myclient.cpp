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

#include <iostream>
#include <sstream>

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

int main (int argc, char **argv) {
    int server_socket_fd;
    char buffer[BUF];
    struct sockaddr_in address;
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
        /*size=recv(server_socket_fd,buffer,BUF-1, 0);
        if (size>0)
        {
            buffer[size]= '\0';
            printf("%s",buffer);
        } */
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

        if(command == "SEND"){
            //read the message
            string string_request,sender,rec,subject,content,attachement,str;
            string_request = "";
            content="";
            char request[1024];
            printf("Sender: ");
            getline(cin,sender);
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
            printf("Path to Attachement(optional): ");
            getline(cin,attachement);
            string_request = "SEND\n"+sender+"\n"+rec+"\n"+subject+"\n"+content+".\n";

            //send the message to the server
            //strcpy(request, string_request.c_str());
            //send(server_socket_fd, request, strlen(request), 0);
            send_message(server_socket_fd,request,string_request.c_str());

            //get OK or ERR back
            //size=recv(server_socket_fd,response,BUF-1, 0);
            //response[size]= '\0';
            receive_message(server_socket_fd,response);

            /*
            if(strncmp(response,"OK",4) == 0)
            {
              send_message(server_socket_fd,request,attachement.c_str());
            }
            */

            /* send file here */

            printf("%s",response);

        }
        else if (command == "LIST"){

            //read the username
            string string_request;
            char request[1024];
            printf("Username: ");
            getline(cin,string_request);
            string_request = "LIST\n"+string_request;

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
        else if(command == "READ"){

            //read the username and number
            string string_request,num;
            char request[1024];
            printf("Username: ");
            getline(cin,string_request);
            printf("Message-Number: ");
            getline(cin,num);
            string_request = "READ\n"+string_request+"\n"+num;

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
        else if(command == "DEL"){

            //read the username and number
            string string_request,num;
            char request[1024];
            printf("Username: ");
            getline(cin,string_request);
            printf("Message-Number: ");
            getline(cin,num);
            string_request = "DEL\n"+string_request+"\n"+num;

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

            cout << "Invalid Input\n";

        }

    }

    close (server_socket_fd);
    return EXIT_SUCCESS;
}
