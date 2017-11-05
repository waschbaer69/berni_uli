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
    server(int port,struct sockaddr_in addr, std::string directory_path);
    ~server();

    void wait_for_request();
private:
    /* attributes */
    int client_socket_fd;
    char buffer[BUF];
    int size;
    bool spool_path_found;
    struct sockaddr_in cliaddress;
    string directory_path;
    string user_id;
    int isLoggedIn = 0;
    int failedLogins = 0;


    /* methods */
    int error(const char* message);
    int receive_message(int socket,char* buffer);
    int recv_file(int sock, char* buffer, string user, string filename);
    string find_user(string id);
    int login_user(string dn, string pwd);
    void server_login();
    void server_send();
    void server_list();
    void server_read();
    void server_del();
    void server_quit();

    void save_message(string now, message m);
    std::vector<message> get_spool(std::string path_to_userdir);

    ssize_t writen (int fd, const char *vptr, size_t n);

};

#endif
