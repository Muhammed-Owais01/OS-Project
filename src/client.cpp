#include "socket.hpp"
#include <iostream>
#include <cstring>
#include <unistd.h>

void send_request(const std::string& request_type) {
    try {
        os_socket::Socket client(AF_INET, SOCK_STREAM, 0);
        client.connect("127.0.0.1", 8080);

        send(client.getSocketFd(), request_type.c_str(), request_type.size(), 0);

        char buffer[1024] = {0};
        recv(client.getSocketFd(), buffer, sizeof(buffer) - 1, 0);
        std::cout << "Response from server:\n" << buffer << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
    }
}

int main() {
    int choice;
    std::cout << "1. Read Data\n";
    std::cout << "2. Write Data\n";
    std::cin >> choice;

    if (choice == 1) {
        send_request("READ");
    } else if (choice == 2) {
        send_request("WRITE");
    } else {
        std::cout << "Invalid choice\n";
    }

    return 0;
}
