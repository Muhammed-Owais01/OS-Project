#include "socket.hpp"
#include "connection_handler.hpp"
#include <unistd.h>

ConnectionHandler::ConnectionHandler(MessageQueue& queue) : queue_(queue) {}

void ConnectionHandler::handle(int client_fd) {
    try {
        // Set timeout directly on the fd (don't create Socket object)
        timeval timeout{.tv_sec = 5, .tv_usec = 0};
        setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

        char buffer[1024] = {0};
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
        
        // Push to queue - processor will be responsible for closing
        queue_.push(client_fd, std::string(buffer));
    } 
    catch (const std::exception& e) {
        std::cerr << "Connection handling error: " << e.what() << "\n";
        close(client_fd);  // Close fd on error
    }
}