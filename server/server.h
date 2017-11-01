// server.h
#ifndef SERVER_H
#define SERVER_H

#define BUF 1024

#include <string>
#include <vector>
#include "../message/message.h"

using namespace std;

class server{
public:
    server(int port, std::string directory_path);
    ~server();

    void wait_for_connection();
    void wait_for_request();
private:
    /* attributes */
    int server_socket_fd, client_socket_fd;
    socklen_t addrlen;
    struct sockaddr_in address, cliaddress;
    char buffer[BUF];
    int size;
    bool spool_path_found;
    string directory_path;

    /* methods */
    int error(const char* message);

    int receive_message(int socket,char* buffer);
    void server_send();
    void server_list();
    void server_read();
    void server_del();
    void server_quit();

    void save_message(message m);
    std::vector<message> get_spool(std::string path_to_userdir);

    ssize_t writen (int fd, const char *vptr, size_t n);

};

#endif
