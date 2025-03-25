#include "socket.hpp"

int main() {
    try {
        os_socket::Socket server(AF_INET, SOCK_STREAM, 0);
        server.bind(8080);
        server.listen(5);

        std::cout << "Server is listening on port 8080...\n";

        while (true) {
            auto [client_fd, client_ip] = server.accept();
            std::cout << "Accepted connection from " << client_ip << "\n";

            std::string response = "HTTP/1.1 200 OK\r\nContent-Length: 13\r\n\r\nHello, world!";
            send(client_fd, response.c_str(), response.size(), 0);
            close(client_fd);
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
    }

    return 0;
}
