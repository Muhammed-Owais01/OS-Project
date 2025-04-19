#include "socket.hpp"
#include <iostream>
#include <string>
#include <stdexcept>
#include <cstring>
#include <cerrno> 

class Client {
private:
    std::string server_ip_;
    int server_port_;
public:
    Client(const std::string& server_ip, int server_port) : server_ip_(server_ip), server_port_(server_port) {}

    std::string sendRequest(const std::string& request) {
        os_socket::Socket socket(AF_INET, SOCK_STREAM, 0);
        socket.setReceiveTimeout(5);
        
        try {
            // Connect to server
            socket.connect(server_ip_, server_port_);
            
            // Send raw command (ensure null termination)
            std::string request_msg = request + '\0';  // Null-terminate the message
            ssize_t sent = send(socket.getSocketFd(), 
                               request_msg.c_str(), 
                               request_msg.size(), 
                               0);
            
            if (sent <= 0) {
                throw std::runtime_error("Send failed");
            }
    
            // Receive response
            char buffer[1024] = {0};
            ssize_t received = recv(socket.getSocketFd(), buffer, sizeof(buffer) - 1, 0);
            
            if (received <= 0) {
                throw std::runtime_error(received == 0 ? "Server disconnected" : "Receive error");
            }
    
            return std::string(buffer, received);
        } catch (const std::exception& e) {
            throw std::runtime_error("Communication error: " + std::string(e.what()));
        }
    }
};

int main() {
    try {
        Client client("127.0.0.1", 8080);

        while (true) {
            std::cout << "1. Read Data\n";
            std::cout << "2. Write Data\n";
            std::cout << "3. Exit\n";
            std::cout << "Choice: ";

            int choice;
            std::cin >> choice;
            std::cin.ignore();

            try {
                switch (choice) {
                    case 1: {
                        std::string response = client.sendRequest("READ");
                        std::cout << "Server response: " << response;
                        break;
                    }
                    case 2: {
                        std::string response = client.sendRequest("WRITE");
                        std::cout << "Server response: " << response;
                        break;
                    }
                    case 3:
                        return 0;
                    default:
                        std::cout << "Invalid choice\n";
                }
            } 
            catch (const std::exception& e) {
                std::cerr << "Error: " << e.what() << "\n";
            }
        }
    } 
    catch (const std::exception& e) {
        std::cerr << "Client fatal error: " << e.what() << "\n";
        return 1;
    }
}