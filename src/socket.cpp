#include "socket.hpp"

namespace os_socket {
    Socket::Socket(int domain, int type, int protocol) 
        : domain(domain), address_len(sizeof(address)) {
        
        sockfd = ::socket(domain, type, protocol);
        if (sockfd == -1) {
            throw std::runtime_error("Socket creation failed: " + std::string(strerror(errno)));
        }
        
        setReuseAddr(true);
    }

    Socket::Socket(int fd) : sockfd(fd) {
        if (sockfd == -1) {
            throw std::runtime_error("Invalid socket file descriptor");
        }
    }

    Socket::~Socket() {
        if (sockfd != -1) {
            shutdown(sockfd, SHUT_RDWR);
            close(sockfd);
        }
    }

    void Socket::setReuseAddr(bool enable) {
        int opt = enable ? 1 : 0;
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            throw std::runtime_error("setsockopt failed: " + std::string(strerror(errno)));
        }
    }

    void Socket::setReceiveTimeout(int seconds) {
        timeval tv;
        tv.tv_sec = seconds;
        tv.tv_usec = 0;
        if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
            throw std::runtime_error("setsockopt timeout failed: " + std::string(strerror(errno)));
        }
    }

    // Binds the socket with the IP and port
    void Socket::bind(int port, const std::string& ip) {
        address.sin_family = domain;
        address.sin_addr.s_addr = ip.empty() ? INADDR_ANY : inet_addr(ip.c_str()); // Holds IP Address, INADDR_ANY = 0.0.0.0, inet_addr converts IP to bits
        address.sin_port = htons(port);

        if (::bind(sockfd, (struct sockaddr*)&address, sizeof(address)) < 0) {
            throw std::runtime_error("Bind failed: " + std::string(strerror(errno)));
        }
    }

    // Marks sockets as ready to accept connectins, backlog is the number of max pending connections queue
    void Socket::listen(int backlog) {
        if (::listen(sockfd, backlog) < 0) {
            throw std::runtime_error("Listen failed: " + std::string(strerror(errno)));
        }
    }

    std::pair<int, std::string> Socket::accept() {
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = ::accept(sockfd, (struct sockaddr*)&client_addr, &client_len);

        if (client_fd < 0) {
            throw std::runtime_error("Accept failed: " + std::string(strerror(errno)));
        }

        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, ip_str, INET_ADDRSTRLEN);

        return {client_fd, std::string(ip_str)};
    }

    void Socket::connect(const std::string& ip, int port) {
        address.sin_family = domain;
        address.sin_port = htons(port);
        // inet_pton converts the character string into a network address structure
        if (inet_pton(domain, ip.c_str(), &address.sin_addr) <= 0) {
            throw std::runtime_error("Invalid address: " + std::string(strerror(errno)));
        }

        if (::connect(sockfd, (struct sockaddr*)&address, sizeof(address)) < 0) {
            throw std::runtime_error("Connection failed: " + std::string(strerror(errno)));
        }
    }
}