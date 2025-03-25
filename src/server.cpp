#include <iostream>
#include <thread>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <semaphore.h>
#include "socket.hpp"

// Shared Resource
int shared_data = 0;
int reader_count = 0;

// Semaphores
sem_t mutex, write_lock;

void reader(Socket client_socket) {
    sem_wait(&mutex);
    reader_count++;
    if (reader_count == 1) {
        // Reader locks writers out
        sem_wait(&write_lock); 
    }
    sem_post(&mutex);

    std::string response = "READ: Shared data = " + std::to_string(shared_data) + "\n";
    send(client_socket.getSocketFd(), response.c_str(), response.size(), 0);

    sem_wait(&mutex);
    reader_count--;
    if (reader_count == 0) {
        // Last reader unlocks writers
        sem_post(&write_lock); 
    }
    sem_post(&mutex);

    close(client_socket.getSocketFd());
}

void writer(Socket client_socket) {
    sem_wait(&write_lock);

    shared_data++;
    std::string response = "WRITE: Shared data updated to " + std::to_string(shared_data) + "\n";
    send(client_socket.getSocketFd(), response.c_str(), response.size(), 0);

    sem_post(&write_lock);
    close(client_socket.getSocketFd());

}

void handle_client(Socket client_socket) {
    char buffer[1024] = {0};

    ssize_t valread = read(client_socket.get_fd(), buffer, sizeof(buffer) - 1);
    if (valread > 0) {
        buffer[valread] = '\0';
        
        if (strncmp(buffer, "READ", 4) == 0) {
            reader(client_socket);
        } else if (strncmp(buffer, "WRITE", 5) == 0) {
            writer(client_socket);
        } else {
            std::string error_msg = "Invalid request\n";
            send(client_socket.getSocketFd(), error_msg.c_str(), error_msg.size(), 0);
            close(client_socket.getSocketFd());
        }
    }
}

int main() {
    sem_init(&mutex, 0, 1);
    sem_init(&write_lock, 0, 1);

    Socket server_socket(AF_INET, SOCK_STREAM, 0);
    server_socket.bind(8080, "127.0.0.1");
    server_socket.listen(10);

    std::vector<std::thread> threads;

    while (true) {
        auto [client_fd, client_ip] = server_socket.accept();
        std::cout << "New connection from: " << client_ip << std::endl;

        threads.emplace_back(handle_client, Socket(client_fd, AF_INET, {}));
    }

    for (auto& t : threads) {
        if (t.joinable()) t.join();
    }

    sem_destroy(&mutex);
    sem_destroy(&write_lock);

    return 0;
}