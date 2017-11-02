// message.cpp
#include <string>
#include <iostream>
#include <sstream>
#include <cstring>
#include <fstream>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctime>

#include "message.h"

using namespace std;

message::message(string completeMessage) {
  //writes the message-content to the class variables
  stringstream messageStream;
  string str;
  messageStream << completeMessage;


  getline(messageStream,str); //already contains SEND

  getline(messageStream,sender);

  getline(messageStream,reciever);

  getline(messageStream,subject);

  while(getline(messageStream,str) && strcmp(str.c_str(),".") != 0){
    content=content+str+"\n";
  }

  getline(messageStream,attachement);


  message_filename = "";

}

message::message(string filename, int file) {
  //writes the message-content to the class variables from a file
  string str;
  stringstream messageStream;

  ifstream in_file (filename);

  getline(in_file,str);
  sender = str.substr(7);
  getline(in_file,str);
  reciever = str.substr(9);
  getline(in_file,str);
  subject = str.substr(8);

  getline(in_file,str); //"message content:"
  while(getline(in_file,str)){
    content=content+str+"\n";
  }

  in_file.close();

  message_filename = filename;

}

message::~message() {
}
//some getters:
string message::get_attachement() {
  if(attachement.find("/")!= std::string::npos) // if string contains filepath
  {
    return attachement.substr((attachement.find_last_of("/")+1),attachement.length()); // just print filename
  }
  else
  {
    return attachement;
  }
}

string message::get_sender() {
  return sender;
}

string message::get_reciever() {
  return reciever;
}

string message::get_subject() {
  return subject;
}

string message::get_content() {
  return content;
}

string message::get_filename() {
  return message_filename;
}
