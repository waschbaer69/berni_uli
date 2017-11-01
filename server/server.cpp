// server.cpp
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <dirent.h>
#include <sys/stat.h>

#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <cstring>
#include <fstream>
#include <chrono>

#define BUF 1024

#include "server.h"
#include "../message/message.h"

using namespace std;

/* constructor */
server::server(int port,string directory_path_string){
    directory_path = directory_path_string;
    if (directory_path.back() != '/') { // last character doesn't equal to '/'
        directory_path += '/';
    }

    DIR* dir = opendir(directory_path.c_str());
    if (dir)
    {
        /* Directory exists. */
        closedir(dir);
    }
    else if (ENOENT == errno)
    {
        /* Directory does not exist. */
        error("directory doesn\'t exist");
        exit(EXIT_FAILURE);
    }
    else
    {
        /* opendir() failed for some other reason. */
        error("directory couldn't be opened");
        exit(EXIT_FAILURE);
    }

    //open the socket for a client to connect

    server_socket_fd = socket (AF_INET, SOCK_STREAM, 0);

    memset(&address,0,sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons (port);

    if (bind ( server_socket_fd, (struct sockaddr *) &address, sizeof (address)) != 0) {
        error("bind error");
    }
    listen (server_socket_fd, 5);

    addrlen = sizeof (struct sockaddr_in);

    spool_path_found = false;

}

server::~server(){
    close (server_socket_fd);
}

int server::error(const char* message){
        //displays an error message
        perror(message);
        return EXIT_FAILURE;
}

int server::receive_message(int socket, char* recv_buffer)
{
  int size_msg = recv (socket, recv_buffer, BUF-1, 0);
  if(size_msg>0) recv_buffer[size_msg]='\0';
  return size_msg;
}

void server::wait_for_connection(){
    //listen until a client connects to the socket

    printf("Waiting for connections...\n");
    client_socket_fd = accept ( server_socket_fd, (struct sockaddr *) &cliaddress, &addrlen );

    if (client_socket_fd > 0){
        printf ("Client connected from %s:%d...\n", inet_ntoa (cliaddress.sin_addr),ntohs(cliaddress.sin_port));

        //send the client a welcome message

        //send_message(client_socket_fd,"Welcome to myserver, Please enter your command:\n");
        strcpy(buffer,"Welcome to myserver, Please enter your command:\n");
        send(client_socket_fd, buffer, strlen(buffer),0);
    }

    //wait until the server recieves a request
    wait_for_request();

}

void server::wait_for_request(){
    //wait for the clients request and deal with it accordingly
    while(1) {
        size = receive_message(client_socket_fd,buffer);
            if(strncmp (buffer, "SEND", 4)  == 0)
            {
                server_send();
            }
            if(strncmp (buffer, "LIST", 4)  == 0)
            {
                server_list();
            }
            if(strncmp (buffer, "READ", 4)  == 0)
            {
                server_read();
            }
            if(strncmp (buffer, "DEL", 3)  == 0)
            {
                server_del();
            }
            if(strncmp (buffer, "QUIT", 4)  == 0)
            {
                server_quit();
                break;
            }
            else if (size > 0)
            {
                //nothing
            }
            else if (size == 0)
            {
                printf("Client closed remote socket\n");
                break;
            }
            else
            {
                error("recv error");
            }
        }

        close (client_socket_fd);

}

void server::server_send(){
    //Senden einer Nachricht vom Client zum Server.
    //read the received buffer and create a message for it
    string mes = buffer;
    message m(mes);
    //check if there's an error (no sender/reciever, char limit broken)
    if(m.get_sender().length() == 0 || m.get_sender().length() > 8 || m.get_reciever().length() == 0  ||  m.get_reciever().length() > 8 || m.get_subject().length() > 80){

        char response[] = "ERR\n\0";
        writen(client_socket_fd,response,strlen(response));

    }else{

        char response[] = "OK\n\0";
        writen(client_socket_fd,response,strlen(response));
        //if everythings ok, save the message
        save_message(m);

    }

}

void server::server_list(){
    //Auflisten der Nachrichten eines Users.

    //scan the received buffer and extract the username from it
    string name = buffer;
    stringstream strs;
    strs << name;
    getline(strs,name); //"LIST"
    getline(strs,name); //username

    //if the username is too long, it doesnt exist
    if(name.length() > 8){
        char response[] = "ERR\n\0";
        writen(client_socket_fd,response,strlen(response));
    }
    else{

        //get a vector with all messages from the user
        string string_path = directory_path+name;
        vector<message> v = get_spool(string_path);

        //if the user doesnt exist, return 0
        if(spool_path_found){

            //number of mesages
            string string_response = to_string(v.size()) + "\n";

            //subject of each message+message number
            for(unsigned int i=0;i<v.size();i++){

                string_response = string_response + to_string(i+1) + ": " + v[i].get_subject() + "\n";

            }

            //send the response to the client
            char response[string_response.size()];
            strcpy(response, string_response.c_str());
            writen(client_socket_fd,response,strlen(response));

        }
        else{

            char response[] = "0\n\0";
            writen(client_socket_fd,response,strlen(response));

        }
    }
}

void server::server_read(){
    //Anzeigen einer bestimmten Nachricht für einen User.

    //scan the received buffer and extract the username and message-number from it
    string name = buffer;
    stringstream strs;
    string string_number;
    strs << name;
    getline(strs,name); //"READ"
    getline(strs,name); //username
    getline(strs,string_number); //message-number

    //if the username is too long or there's no number, send an error
    if(name.length() > 8 || name.length() == 0 || string_number.length() == 0){
        char response[] = "ERR\n\0";
        writen(client_socket_fd,response,strlen(response));
    }
    else{
        //see if there is an acutal number entered
        unsigned int number = 0;
        try{
            number = stoi(string_number);
        }
        catch (const exception& e) {
            number = 0; //0 wirtes back ERR later
        }

        //get a vector with all messages from the user
        string string_path = directory_path+name;
        vector<message> v = get_spool(string_path);

        //if the user doesnt exist, return ERR
        if(spool_path_found && v.size() >= number && number > 0){
            //get the message with the input message-number
            string string_response;
            string_response = "\n"+v[number-1].get_sender()+"\n"+v[number-1].get_reciever()+"\n"+v[number-1].get_subject()+"\n"+v[number-1].get_content()+"\n";

            //send the message back
            char response[string_response.size()];
            strcpy(response, string_response.c_str());
            writen(client_socket_fd,response,strlen(response));
        }
        else {

            char response[] = "ERR\n\0";
            writen(client_socket_fd,response,strlen(response));

        }
    }
}

void server::server_del(){
    //Löschen einer Nachricht eines Users.

    //scan the received buffer and extract the username and message-number from it
    string name = buffer;
    stringstream strs;
    string string_number;
    strs << name;
    getline(strs,name); //"DEL"
    getline(strs,name); //username
    getline(strs,string_number); //mess-number

    //if the username is too long or there's no number, send an error
    if(name.length() > 8 || name.length() == 0 || string_number.length() == 0){
        char response[] = "ERR\n\0";
        writen(client_socket_fd,response,strlen(response));
    }
    else{
        //see if there is an acutal number entered
        unsigned int number = 1337;
        try{
            number = stoi(string_number);
        }
        catch (const exception& e) {
            name = "someusernamethatcanneverbetankenandthereforeproducesanerror"; //stupid but it works
        }

        //get a vector with all messages from the user
        string string_path = directory_path+name;
        vector<message> v = get_spool(string_path);

        //if the user doesnt exist, return ERR
        if(spool_path_found && v.size() >= number && number > 0){

            //get the filename of the message and remove it
            string string_fn = v[number-1].get_filename();
            char fn[string_fn.size()];
            strcpy(fn, string_fn.c_str());
            remove(fn);

            char response[] = "OK\n\0";
            writen(client_socket_fd,response,strlen(response));

        }
        else{

            char response[] = "ERR\n\0";
            writen(client_socket_fd,response,strlen(response));

        }
    }
}

void server::server_quit(){
    //Logout des Clients

    printf ("Client closed connection from %s:%d...\n", inet_ntoa (cliaddress.sin_addr),ntohs(cliaddress.sin_port));

}

void server::save_message(message m){
    //save to file

    //create a new directory for the sender/user
    string string_path = directory_path+m.get_reciever();

    char path[1024];
    strcpy(path, string_path.c_str());
    mkdir(path, 0700);


    //generate a new filename by taking time since epoch + receiver+subject
    long long int now = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
    string filename = to_string(now)+"_"+m.get_reciever()+"_"+m.get_subject()+".txt";

    //create the file and its content
    ofstream outfile (string_path+"/"+filename);

    outfile << "Sender:" << m.get_sender() << endl;
    outfile << "Receiver:" << m.get_reciever() << endl;
    outfile << "Subject:" << m.get_subject() << endl;
    outfile << "Message Content:\n" << m.get_content();

    outfile.close();

}

vector<message> server::get_spool(string path_to_userdir){
    //create a vector that contains all messages from a specific user

    vector<message> spool;

    //get the user_path given as parameter
    char path[1024];
    strcpy(path, path_to_userdir.c_str());

    //open the directory
    DIR *dir;
    struct dirent *ent;
    //if there's no such directory, set a boolean to false to produce an error in the method that called get_spool()
    if ((dir = opendir (path)) != NULL){
        spool_path_found = true;
        while ((ent = readdir (dir)) != NULL) {
            if(*ent->d_name != '.'){
                //create a new message-object for each found file
                string filename = path_to_userdir + "/" + ent->d_name;
                spool.push_back(message(filename,1));
            }
        }
        closedir (dir);
    }
    else{
        spool_path_found = false;
        //error("directory not found");
    }

    return spool;

}

ssize_t server::writen (int fd, const char *vptr, size_t n){
    //taken from tcpip_linux-prog-details.pdf, writes n bytes to a descriptor

    size_t      nleft;
    ssize_t     nwritten;
    const char  *ptr;
    ptr = vptr;
    nleft = n;
    while (nleft > 0) {
        if ( (nwritten = write(fd,ptr,nleft)) <= 0) {
            if (errno == EINTR)
            nwritten = 0;         // and call write() again
            else
            return -1;//retung (-1);
        }
        nleft -= nwritten;
        ptr += nwritten;
    }
    return (n);
}
