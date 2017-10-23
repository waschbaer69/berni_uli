#include "Mserver.h"

namespace fs = std::experimental::filesystem;

MServer::MServer()
{
  //Buffer wird initialisiert und der readbuffer "geleert"
  buffer = "";
  memset(&readbuffer,0, sizeof(readbuffer));
}

void MServer::startserver(char **argv) 
{
  create_socket = socket (AF_INET, SOCK_STREAM, 0);
  if (create_socket < 0) 
  {
    throw exception();
  }
  port = stoi(argv[2]);
  memset(&address,0,sizeof(address));
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons (port);

  /*
      Wenn der Server gestartet wurde -> bind() - listen() - accept()
  */

  // bind()
  if(bind ( create_socket, (struct sockaddr *) &address, sizeof (address)) != 0) 
  {
     perror("bind error");
     throw exception();
  }

  //listen()
  listen (create_socket, 5);
  addrlen = sizeof (struct sockaddr_in);

  /*
    Die Vorlage auf dem Moodle wurde benützt und modifiziert
  */
  while (1) {
     printf("Waiting for connections...\n");
     //accept()
     new_socket = accept ( create_socket, (struct sockaddr *) &cliaddress, &addrlen );

     if (new_socket > 0)
     {
        printf ("Client connected from %s:%d...\n", inet_ntoa (cliaddress.sin_addr),ntohs(cliaddress.sin_port));
        buffer = "Welcome to your E-Mail Client, Please enter your command:\n";
        send(new_socket,buffer.c_str(), buffer.length(),0);
        buffer = "";
     }
     do {
        //RECEIVE COMMAND -> Siehe client.cpp
        while((size = recv(new_socket, readbuffer,BUF-1, 0)) > 0)
        {
          buffer.append(readbuffer, size);

          if(size < BUF-1) break;
        } 
        
        /* Get command from messagebuffer */
        string command;

        istringstream ss(buffer);
        getline(ss, command, ':');
        
        //Das hier war eine Testausgabe
        //cout << command << endl;
        
        string username;
        string msge;

        //Parameter -> directory
        directory = argv[1];
        if( size > 0 )
        {
          if(command == "SEND")
          {
            /* Split Command from message */
            msge = buffer;
            msge = msge.substr(5, msge.length());

            //save message as file
            savemail(msge);
            memset(&readbuffer,0,sizeof(readbuffer));
          }                 
          else if(command == "LIST")
          {
            msge = buffer;
            username = msge.substr(5, msge.length());

            //send list to user
            list(username);
            memset(&readbuffer,0,sizeof(readbuffer));
          }
          else if(command == "READ")
          {
            string tmp = "";
            msge = buffer;
            istringstream ss(msge);

            //get username (dir)
            getline(ss, username, ':');
            getline(ss, username, '\n');
            
            //get nachrichtenID (die ich lesen will)
            getline(ss, tmp, '\n');
            int nID = stoi(tmp);

            //send email to user
            reading(username, nID);
            memset(&readbuffer,0,sizeof(readbuffer));
          }        
          else if(command == "DELE") 
          {
            string tmp = "";
            msge = buffer;
            istringstream ss(msge);

            //get username (dir)
            getline(ss, username, ':');
            getline(ss, username, '\n');
            
            //get nachrichtenID (die ich löschen will)
            getline(ss, tmp, '\n');
            int nID = stoi(tmp);

            //send OK OR ERR to user
            del(username, nID);
            memset(&readbuffer,0,sizeof(readbuffer));
          }
          else 
          {
            cout << "command does not exist" << endl;
          }

        }
        else if (size == 0)
        {
          cout << "Client closed remote socket\n";
          break;
        }
        else
        {
          perror("recv error");
          throw exception();
        }
     } while (strncmp (buffer.c_str(), "quit", 4)  != 0);
     close (new_socket);
  }
  close (create_socket);
}

/*************************** SAVEMAIL ***************************/
void MServer::savemail(string msg) 
{
  ofstream savingmail;

  /* Erstellt ein Message-Objekt und trennt Sender,Empfänger usw. */
  Mmessage fullmsg(msg);

  /* Erstellt das User Verzeichnis, in dem die Mails gespeichert werden */
  string userdir = directory + "/" + fullmsg.get_empf();

  //make directory with name of empfaenger
  mkdir(userdir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
  
  /* Neue MailID bekommen */
  ifstream mail;
  string path = directory+"/"+fullmsg.get_empf();
  int mailID = 1;

  /* */
  if(fs::exists(path))
  {
    for(auto & p : fs::directory_iterator(path))
    {
      mail.clear();
      mail.open(p.path().string());

      if(mail.fail()) 
      {
        throw runtime_error("File does not exist!");
      }
      else 
      {
        string tmp = "";
        getline(mail, tmp);
        int num = stoi(tmp);
        if(mailID <= num) 
        {
          mailID = num;
          mailID++;
        }
      }
      mail.close();
    }
  }
  else
  {
    buffer = "ERR\nUser not found";
  }



  /* TIMESTAMP */

  time_t rawtime;
  struct tm * timeinfo;
  char timebuf[80];

  time (&rawtime);
  timeinfo = localtime(&rawtime);

  strftime(timebuf,sizeof(timebuf),"%d-%m-%Y$%I_%M_%S",timeinfo);
  string timestamp(timebuf);

  /* END OF TIMESTAMP */


  string filename = directory+"/"+fullmsg.get_empf()+"/"+fullmsg.get_empf()+"_"+timestamp+".txt";


  savingmail.open(filename.c_str());

  if(savingmail.is_open()) 
  { 
    savingmail << mailID << "\r\n";
    savingmail << fullmsg.get_sender() << "\r\n";
    savingmail << fullmsg.get_empf() << "\r\n";
    savingmail << fullmsg.get_betreff() << "\r\n";
    savingmail << fullmsg.get_nachricht() << "\r\n";
  }
  buffer = "OK\n";
  send(new_socket, buffer.c_str(), buffer.length(),0);
  buffer = "";
}

/*************************** READMAIL ***************************/
void MServer::reading(string username, int nachrichtID)
{
  buffer = "";
  ifstream mail;
  string folderpath = directory+"/"+username;
  stringstream ss;
  int checkid = 0;
  string str = "";
  if(fs::exists(folderpath))
  {
    for(auto & p : fs::directory_iterator(folderpath))
    {
      mail.clear();
      mail.open(p.path().string());
      
      if(mail.fail()) 
      {
        throw runtime_error("File does not exist!");
      }
      else 
      {
        string tmp = "";

        getline(mail, tmp);
        checkid = stoi(tmp);
        if(checkid == nachrichtID) 
        {
           str.assign((std::istreambuf_iterator<char>(mail)),
                           std::istreambuf_iterator<char>());
          buffer = str;
          break;
        }
        else 
        {
          buffer = "ERR\n";
        }
      }
      mail.close();
    }
  }
  else
  {
    buffer = "ERR\nUser not found\n";
  }
  send(new_socket, buffer.c_str(), buffer.length(),0);
  buffer = "";
}

/*************************** LISTMAILS ***************************/
void MServer::list(string username)
{
  buffer = "";
  ifstream mail;
  string folderpath = directory+"/"+username;
  stringstream ss;

  if(fs::exists(folderpath))
  {
    for(auto & p : fs::directory_iterator(folderpath))
    {
      mail.clear();
      mail.open(p.path().string());
      if(!mail.is_open()) 
      {
        cout << "HIER !!!" << endl;
      }
      else 
      {
        string tmp = "";

        getline(mail, tmp);
        int num = stoi(tmp);
        ss << num << ". ";
        getline(mail, tmp);
        getline(mail, tmp);
        getline(mail, tmp);

        ss << tmp << "\n";
      }
      mail.close();
    }
  }
  else
  {
    ss << "ERR\n";
  }

  send(new_socket, ss.str().c_str(), ss.str().length(),0);
  buffer = "";
}

/*************************** DELETEMAIL ***************************/
void MServer::del(string username, int nachrichtID)
{
  buffer = "";
  ifstream mail;
  string folderpath = directory+"/"+username;
  int checkid = 0;

  if(fs::exists(folderpath))
  {
    for(auto & p : fs::directory_iterator(folderpath))
    {
      mail.clear();
      mail.open(p.path().string());
      
      if(mail.fail()) 
      {
        throw runtime_error("File does not exist!");
      }
      else 
      {
        string tmp = "";

        getline(mail, tmp);
        checkid = stoi(tmp);
        if(checkid == nachrichtID) 
        {
          remove(p);
          buffer = "OK\n";
          break;
        } 
        else 
        {
          buffer = "ERR\n";
        }
      }
      mail.close();
    }
  }
  else
  {
    buffer = "ERR\n";
  }
  send(new_socket, buffer.c_str(), buffer.length(),0);
  buffer = "";
}




