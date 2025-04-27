#include "socket.hpp"
#include "connection_handler.hpp"
#include "http_parser.hpp"
#include <unistd.h>
#include <cstring>

ConnectionHandler::ConnectionHandler(MessageQueue& queue) : queue_(queue) {}

void ConnectionHandler::handle(int client_fd) {
    try {
        // Set timeout directly on the fd (don't create Socket object)
        timeval timeout{.tv_sec = 5, .tv_usec = 0};
        setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

        char buffer[4096] = {0};
        ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

        if (bytes_read <= 0) {
            if (bytes_read == 0) {
                std::cerr << "Client disconnected\n";
            } else {
                std::cerr << "Receive error: " << strerror(errno) << "\n";
            }
            close(client_fd);  // Close fd ourselves
            return;
        }

        buffer[bytes_read] = '\0';
        std::string raw_request(buffer);
        auto request_opt = HttpParser::parse(raw_request);
        if(!request_opt){
            std::string response = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nContent-Length: 11\r\nConnection: close\r\n\r\nBad Request";
            send(client_fd, response.c_str(), response.size(), 0);
            close(client_fd);
            
            return;
        }
        // Push to queue - processor will be responsible for closing
        queue_.push(client_fd, raw_request);
    } 
    catch (const std::exception& e) {
        std::cerr << "Connection handling error: " << e.what() << "\n";
        close(client_fd);  // Close fd on error
    }
}