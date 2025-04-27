#include "socket.hpp"
#include <iostream>
#include <string>
#include <stdexcept>
#include <cstring>
#include <cerrno> 
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class Client {
private:
    std::string server_ip_;
    int server_port_;
public:
    Client(const std::string& server_ip, int server_port) : server_ip_(server_ip), server_port_(server_port) {}

    std::string sendRequest(const std::string& method, const std::string& path, const std::string& body = "") {
        os_socket::Socket socket(AF_INET, SOCK_STREAM, 0);
        socket.setReceiveTimeout(5);
        
        try {
            socket.connect(server_ip_, server_port_);
            
            std::string request = method + " " + path + " HTTP/1.1\r\n";
            request += "Host: " + server_ip_ + "\r\n";
            request += "Content-Type: application/json\r\n";
            if (!body.empty()) {
                request += "Content-Length: " + std::to_string(body.size()) + "\r\n";
            }
            request += "Connection: close\r\n\r\n";
            if (!body.empty()) {
                request += body;
            }
            
            ssize_t sent = send(socket.getSocketFd(), request.c_str(), request.size(), 0);
            if (sent <= 0) {
                throw std::runtime_error("Send failed");
            }
    
            char buffer[4096] = {0};
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
            std::cout << "1. GET /users\n";
            std::cout << "2. POST /users\n";
            std::cout << "3. Exit\n";
            std::cout << "Choice: ";

            int choice;
            std::cin >> choice;
            std::cin.ignore();

            try {
                switch (choice) {
                    case 1: {
                        std::string response = client.sendRequest("GET", "/users");
                        std::cout << "Server response:\n" << response << "\n";
                        break;
                    }
                    case 2: {
                        std::cout << "Enter name: ";
                        std::string name;
                        std::getline(std::cin, name);
                        std::cout << "Enter email: ";
                        std::string email;
                        std::getline(std::cin, email);

                        json user = {{"name", name}, {"email", email}};
                        std::string body = user.dump();
                        std::string response = client.sendRequest("POST", "/users", body);
                        std::cout << "Server response:\n" << response << "\n";
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