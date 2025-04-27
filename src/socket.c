#include "socket.h"
#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>

int socket_init(Socket* sock, int domain, int type, int protocol) {
    sock->sockfd = socket(domain, type, protocol);
    if (sock->sockfd == -1) {
        fprintf(stderr, "Socket creation failed: %s\n", strerror(errno));
        return -1;
    }
    
    sock->domain = domain;
    sock->address_len = sizeof(sock->address);
    
    if (socket_set_reuse_addr(sock, true) != 0) {
        close(sock->sockfd);
        return -1;
    }
    
    return 0;
}

int socket_init_from_fd(Socket* sock, int fd) {
    if (fd == -1) {
        fprintf(stderr, "Invalid socket file descriptor\n");
        return -1;
    }
    sock->sockfd = fd;
    return 0;
}

void socket_destroy(Socket* sock) {
    if (sock->sockfd != -1) {
        shutdown(sock->sockfd, SHUT_RDWR);
        close(sock->sockfd);
        sock->sockfd = -1;
    }
}

int socket_set_reuse_addr(Socket* sock, bool enable) {
    int opt = enable ? 1 : 0;
    if (setsockopt(sock->sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        fprintf(stderr, "setsockopt failed: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

int socket_set_receive_timeout(Socket* sock, int seconds) {
    struct timeval tv;
    tv.tv_sec = seconds;
    tv.tv_usec = 0;
    if (setsockopt(sock->sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        fprintf(stderr, "setsockopt timeout failed: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

int socket_bind(Socket* sock, int port, const char* ip) {
    sock->address.sin_family = sock->domain;
    sock->address.sin_port = htons(port);
    
    if (ip == NULL || strcmp(ip, "0.0.0.0") == 0) {
        sock->address.sin_addr.s_addr = INADDR_ANY;
    } else {
        sock->address.sin_addr.s_addr = inet_addr(ip);
    }

    if (bind(sock->sockfd, (struct sockaddr*)&sock->address, sizeof(sock->address)) < 0) {
        fprintf(stderr, "Bind failed: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

int socket_listen(Socket* sock, int backlog) {
    if (listen(sock->sockfd, backlog) < 0) {
        fprintf(stderr, "Listen failed: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

int socket_accept(Socket* sock, int* client_fd, char* client_ip) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    *client_fd = accept(sock->sockfd, (struct sockaddr*)&client_addr, &client_len);
    if (*client_fd < 0) {
        fprintf(stderr, "Accept failed: %s\n", strerror(errno));
        return -1;
    }

    if (client_ip != NULL) {
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
    }
    
    return 0;
}

int socket_connect(Socket* sock, const char* ip, int port) {
    sock->address.sin_family = sock->domain;
    sock->address.sin_port = htons(port);
    
    if (inet_pton(sock->domain, ip, &sock->address.sin_addr) <= 0) {
        fprintf(stderr, "Invalid address: %s\n", strerror(errno));
        return -1;
    }

    if (connect(sock->sockfd, (struct sockaddr*)&sock->address, sizeof(sock->address)) < 0) {
        fprintf(stderr, "Connection failed: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

int socket_get_fd(const Socket* sock) {
    return sock->sockfd;
}