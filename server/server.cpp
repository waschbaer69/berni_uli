// server.cpp
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h> /* open, O_RDONLY */

#include <dirent.h>
#include <sys/stat.h>

#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <cstring>
#include <fstream>
#include <chrono>
#include <ldap.h>
#include <pthread.h>

#define BUF 1024
#define LDAP_HOST "ldap.technikum-wien.at"
#define LDAP_PORT 389
#define FILTER "(uid=if16b093)"
#define SEARCHBASE "dc=technikum-wien,dc=at"
#define SCOPE LDAP_SCOPE_SUBTREE
/* anonymous bind with user and pw NULL (you must be connected to fh-network),
else enter your credentials here*/
#define BIND_USER NULL//"uid=if16b502,ou=people,dc=technikum-wien,dc=at"
#define BIND_PW NULL //"pwd"
#define BANTIME 30 //bantime in minutes

#include "server.h"
#include "../message/message.h"

using namespace std;
//pthread_mutex_t mutex;

/* constructor */
server::server(int client_socket,struct sockaddr_in addr, string directory_path_string){

  client_socket_fd = client_socket;
  cliaddress = addr;
  isLoggedIn = 0;

  //pthread_mutex_init (&mutex, NULL);

  directory_path = directory_path_string;
  if (directory_path.back() != '/')
  { // last character doesn't equal to '/'
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

  spool_path_found = false;
}

server::~server()
{
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

void server::wait_for_request(){
  //wait for the clients request and deal with it accordingly
  while(1) {
    size = receive_message(client_socket_fd,buffer);
    if(strncmp (buffer,"LOGIN", 5) == 0)
    {
        server_login();
        if(failedLogins > 2) {
          break;
        }
    }
    if((strncmp (buffer, "SEND", 4)  == 0) && (isLoggedIn == 1))
    {
      server_send();
    }
    if((strncmp (buffer, "LIST", 4)  == 0) && (isLoggedIn == 1))
    {
      server_list();
    }
    if((strncmp (buffer, "READ", 4)  == 0) && (isLoggedIn == 1))
    {
      server_read();
    }
    if((strncmp (buffer, "DEL", 3)  == 0) && (isLoggedIn == 1))
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
  printf("Closed Socket\n");
}

string server::find_user(string id)
{
  string filter_str = "(uid=" + id + ")";
  printf("%s\n",filter_str.c_str());
  string user_dn;
  LDAP *ld;			/* LDAP resource handle */
  LDAPMessage *result, *e;	/* LDAP result handle */


  int rc=0;

  char *attribs[3];		/* attribute array for search */

  attribs[0]=strdup("uid");		/* return uid and cn of entries */
  attribs[1]=strdup("cn");
  attribs[2]=NULL;		/* array must be NULL terminated */

  /* setup LDAP connection */
  if ((ld=ldap_init(LDAP_HOST, LDAP_PORT)) == NULL)
  {
    perror("ldap_init failed");
    user_dn = "ERR";
  }

  //printf("connected to LDAP server %s on port %d\n",LDAP_HOST,LDAP_PORT);

  /* anonymous bind */
  rc = ldap_simple_bind_s(ld,BIND_USER,BIND_PW);

  if (rc != LDAP_SUCCESS)
  {
    fprintf(stderr,"LDAP error: %s\n",ldap_err2string(rc));
    user_dn = "ERR";
  }
  else
  {
    //printf("bind successful\n");
  }

  /* perform ldap search */
  rc = ldap_search_s(ld, SEARCHBASE, SCOPE, filter_str.c_str(), attribs, 0, &result);

  if (rc != LDAP_SUCCESS)
  {
    fprintf(stderr,"LDAP search error: %s\n",ldap_err2string(rc));
    user_dn = "ERR";
  }

  //printf("Total results: %d\n", ldap_count_entries(ld, result));

  if(ldap_count_entries(ld, result) == 1)
  {
    /* get user DN */
    for (e = ldap_first_entry(ld, result); e != NULL; e = ldap_next_entry(ld,e))
    {
      //printf("DN: %s\n", ldap_get_dn(ld,e));
      user_dn = ldap_get_dn(ld,e);
      //printf("\n");
    }
  }
  else
  {
    user_dn = "ERR";
  }

  /* free memory used for result */
  ldap_msgfree(result);
  free(attribs[0]);
  free(attribs[1]);
  //printf("LDAP search suceeded\n");

  ldap_unbind(ld);

  return user_dn;
}

int server::login_user(string dn, string pwd)
{
  LDAP *ld;			/* LDAP resource handle */
  int rc=0;
  int success = 1;


  /* setup LDAP connection */
  if ((ld=ldap_init(LDAP_HOST, LDAP_PORT)) == NULL)
  {
    perror("ldap_init failed");
    success = 0;
  }

  //printf("connected to LDAP server %s on port %d\n",LDAP_HOST,LDAP_PORT);

  /* anonymous bind */
  rc = ldap_simple_bind_s(ld,dn.c_str(),pwd.c_str());

  if (rc != LDAP_SUCCESS)
  {
    fprintf(stderr,"LDAP error: %s\n",ldap_err2string(rc));
    success = 0;
    failedLogins++;
  }
  else
  {
    printf("User could be logged in\n");
    isLoggedIn = 1; // set flag to true
  }

  ldap_unbind(ld);
  return success;

}

void server::server_login()
{
  string uid,pwd;

  string msg = buffer;
  stringstream strs;
  strs << msg;
  getline(strs,msg); //"LOGIN"
  getline(strs,uid); //uid
  getline(strs,pwd); //pwd

  //printf("%s\n",uid.c_str());
  //printf("%s\n",pwd.c_str());
  string dn = find_user(uid).c_str();

  if(dn.compare("ERR") != 0)
  {
    printf("Found User: %s\n",dn.c_str());
    int isValid = login_user(dn,pwd);
    if(isValid == 1)
    {
      user_id = uid;
      printf("%s", user_id.c_str());
      char response[] = "OK\n\0";
      writen(client_socket_fd,response,strlen(response));
    }
    else
    { //wrong credentials
      //if wrong 3rd time ban

      char response[100] = "ERR\n\0";
      if(failedLogins > 2) {
        //notify of ban
        sprintf(response, "ERR\nYour IP %s has been banned for %d minute(s)\n", inet_ntoa(cliaddress.sin_addr), BANTIME);
        printf("%s", response);
        //write to blacklist

        unsigned long now = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count() + BANTIME*60000;
        char address[100];
        sprintf(address, "%s", inet_ntoa(cliaddress.sin_addr));

        ofstream outfile (directory_path+"/blacklist");
        outfile << address << " " << to_string(now) << endl;
        outfile.close();
      }
      writen(client_socket_fd,response,strlen(response));
    }
  }
  else
  {
    char response[] = "ERR\n\0";
    writen(client_socket_fd,response,strlen(response));
  }
}

int server::recv_file(int client_socket, char* buffer, string user, string filename)
{
  int file_size;
  int recv_error = 0;
  FILE *received_file;
  ssize_t len;
  int remain_data = 0;

  /* Receiving file size */
  recv(client_socket, buffer, BUF, 0);
  file_size = atoi(buffer);
  if(file_size <= 0)
  {
    recv_error = 1;
  }

  if(file_size > 0) // only if file is valid, recv message
  {
    /* create directory */
    string string_path = directory_path + user;
    char path[1024];
    strcpy(path, string_path.c_str());
    mkdir(path, 0700);

    /* create file */
    string complete_path = directory_path + user + "/" + filename;
    //printf("%s\n",complete_path.c_str());
    received_file = fopen(complete_path.c_str(), "w");

    if (received_file == NULL)
    {
      fprintf(stderr, "Failed to open file foo --> %s\n", strerror(errno));
      recv_error= 1;
    }

    remain_data = file_size;

    while (remain_data > 0)
    {
      len = recv(client_socket, buffer, BUF, 0);
      if(recv_error == 0) fwrite(buffer, sizeof(char), len, received_file);
      remain_data -= len;
      //fprintf(stdout, "Receive %d bytes and we hope :- %d bytes\n", (int) len, remain_data);
    }

    fclose(received_file); // segmentation fault verhindern
  }


  return recv_error;
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



    // If Thread 1 sets the mutex and wants to create a file name, all other threads can't access variable "now"
    // -> the filename will always be unique
    //pthread_mutex_lock (&mutex);
    /*
    if (pthread_mutex_trylock(&mutex) == 0)
    {
      printf("Mutex got locked\n");
    }
    else
    {
      printf("This thread doesn't own the lock\n");
      /// Fail!  This thread doesn't own the lock.  Do something else...
    } */
    fprintf(stdout, "thread:%ld\n",pthread_self());
    long long int now = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
    string file_name = to_string(now)+"_"+"atm"+"_"+m.get_attachement();
    //pthread_mutex_unlock (&mutex);

    int recv_err = recv_file(client_socket_fd,buffer,m.get_reciever(), file_name);
    if(recv_err == 0)
    {
      //if everythings ok, save the message
      save_message(to_string(now),m);
      char response[] = "OK\n\0";
      writen(client_socket_fd,response,strlen(response));
    }
    else
    {
      char response[] = "ERR\n\0";
      writen(client_socket_fd,response,strlen(response));
    }
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

void server::save_message(string now,message m){
  //save to file

  //create a new directory for the sender/user
  string string_path = directory_path+m.get_reciever();

  char path[1024];
  strcpy(path, string_path.c_str());
  mkdir(path, 0700);


  //generate a new filename by taking time since epoch + receiver+subject
  string filename = now+".txt";

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
        string file_only = ent->d_name;
        if (file_only.find("atm") != std::string::npos) {
          // don't push back attachements
        }
        else
        {
          spool.push_back(message(filename,1));
        }
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
