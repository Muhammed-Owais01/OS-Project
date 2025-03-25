#pragma once

#ifndef SOCKET_H
#define SOCKET_H

#include <iostream>
#include <string>
#include <utility>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

namespace os_socket {  
    typedef struct sockaddr_in serv_addr_in;

    class Socket {
    private:
        int sockfd = -1;
        int option = 1;
        int domain;
        serv_addr_in address;
        socklen_t address_len;

    public:
        Socket(int domain, int type, int protocol = 0);
        ~Socket();

        void bind(int port, const std::string& ip = "0.0.0.0");
        void listen(int backlog = 5);
        std::pair<int, std::string> accept();
        void connect(const std::string& ip, int port);

        int getSocketFd() const;
    };
}

#endif
