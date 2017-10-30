// message.h

#ifndef VSYS_MESSAGE_H
#define VSYS_MESSAGE_H

#include <string>

using namespace std;

class message {
private:
    string sender;
    string reciever;
    string subject;
    string content;
    string message_filename;
public:
    message(string completeMessage);
    message(string messageSender, int file);
    ~message();

    string get_sender();
    string get_reciever();
    string get_subject();
    string get_content();

    string get_filename();

    void save();

};

#endif //VSYS_MESSAGE_H
