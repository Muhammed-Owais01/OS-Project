#include "socket.hpp"

namespace os_socket {
    Socket::Socket(int domain, int type, int protocol) : domain(domain), address_len(sizeof(address)) {
        sockfd = ::socket(domain, type, protocol);
        if (sockfd == -1) {
            throw std::runtime_error("Socket creation failed");
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) < 0) {
            throw std::runtime_error("setsockopt failed");
        }
    }

    Socket::~Socket() {
        if (sockfd != -1) {
            close(sockfd);
        }
    }

    void Socket::bind(int port, const std::string& ip) {
        address.sin_family = domain;
        address.sin_addr.s_addr = inet_addr(ip.c_str());
        address.sin_port = htons(port);

        if (::bind(sockfd, (struct sockaddr*)&address, sizeof(address)) < 0) {
            throw std::runtime_error("Bind failed");
        }
    }

    void Socket::listen(int backlog) {
        if (::listen(sockfd, backlog) < 0) {
            throw std::runtime_error("Listen failed");
        }
    }

    std::pair<int, std::string> Socket::accept() {
        serv_addr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = ::accept(sockfd, (struct sockaddr*)&client_addr, &client_len);

        if (client_fd < 0) {
            throw std::runtime_error("Accept failed");
        }

        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, ip_str, INET_ADDRSTRLEN);

        return {client_fd, std::string(ip_str)};
    }

    void Socket::connect(const std::string& ip, int port) {
        address.sin_family = domain;
        address.sin_port = htons(port);

        if (inet_pton(domain, ip.c_str(), &address.sin_addr) <= 0) {
            throw std::runtime_error("Invalid address");
        }

        if (::connect(sockfd, (struct sockaddr*)&address, sizeof(address)) < 0) {
            throw std::runtime_error("Connection failed");
        }
    }

    int Socket::getSocketFd() const {
        return sockfd;
    }
}
