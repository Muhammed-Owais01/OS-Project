#ifndef SOCKET_HPP
#define SOCKET_HPP
#pragma once
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdexcept>
#include <string>
#include <iostream>
#include <cstring>
#include <cerrno> 

namespace os_socket {
    class Socket {
    private:
        int sockfd;
        int domain;
        sockaddr_in address;
        socklen_t address_len;
        int option = 1;
    public:
        Socket(int domain, int type, int protocol);
        Socket(int fd);
        ~Socket();
        
        void bind(int port, const std::string& ip = "0.0.0.0");
        void listen(int backlog = 10);
        std::pair<int, std::string> accept();
        void connect(const std::string& ip, int port);
        
        int getSocketFd() const { return sockfd; }
        
        // Additional helper methods
        void setReceiveTimeout(int seconds);
        void setReuseAddr(bool enable);
    };
}

#endif // SOCKET_HPP