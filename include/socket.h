#ifndef SOCKET_H
#define SOCKET_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

typedef struct {
    int sockfd;
    int domain;
    struct sockaddr_in address;
    socklen_t address_len;
} Socket;

int socket_init(Socket* sock, int domain, int type, int protocol);
int socket_init_from_fd(Socket* sock, int fd);
void socket_destroy(Socket* sock);

int socket_bind(Socket* sock, int port, const char* ip);
int socket_listen(Socket* sock, int backlog);
int socket_accept(Socket* sock, int* client_fd, char* client_ip);
int socket_connect(Socket* sock, const char* ip, int port);

int socket_set_receive_timeout(Socket* sock, int seconds);
int socket_set_reuse_addr(Socket* sock, bool enable);

int socket_get_fd(const Socket* sock);

#endif // SOCKET_H