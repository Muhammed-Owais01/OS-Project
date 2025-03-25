#include "socket.hpp"

int main() {
    try {
        os_socket::Socket client(AF_INET, SOCK_STREAM, 0);
        client.connect("127.0.0.1", 8080);

        std::string request = "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n";
        send(client.getSocketFd(), request.c_str(), request.size(), 0);

        char buffer[1024] = {0};
        recv(client.getSocketFd(), buffer, sizeof(buffer) - 1, 0);
        std::cout << "Response from server:\n" << buffer << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
    }

    return 0;
}
