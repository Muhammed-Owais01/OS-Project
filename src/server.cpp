#include "socket.hpp"
#include <iostream>
#include <thread>
#include <vector>
#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <cstring>
#include <cerrno>

class LockGuard {
private:
    std::mutex& mutex_;
public:
    explicit LockGuard(std::mutex& mutex) : mutex_(mutex) {
        mutex_.lock();
    }
    
    ~LockGuard() {
        mutex_.unlock();
    }
};

class ThreadSafeData {
private:
    int data_ = 0;
    mutable std::mutex mutex_;
public:
    int read() const {
        LockGuard lock(mutex_);
        return data_;
    }

    void write(int value) {
        LockGuard lock(mutex_);
        data_ = value;
    }
};

class ConnectionHandler {
private:
    ThreadSafeData& shared_data_;
public:
    ConnectionHandler(ThreadSafeData& shared_data) : shared_data_(shared_data) {}

    void handle(int client_fd) {
        try {
            // Creates client socket
            os_socket::Socket client_socket(client_fd);
            // Timeout to prevent hanging 
            client_socket.setReceiveTimeout(5);

            char buffer[1024] = {0};
            ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

            if (bytes_read <= 0) {
                if (bytes_read == 0) {
                    std::cerr << "Client disconnected\n";
                } else {
                    std::cerr << "Receive error: " << strerror(errno) << "\n";
                }
                return;
            }

            buffer[bytes_read] = '\0';
            // Converts the read char array into a string
            std::string request(buffer);

            std::string response;
            if (request == "READ") {
                response = "DATA: " + std::to_string(shared_data_.read()) + "\n";
            } 
            else if (request == "WRITE") {
                int current = shared_data_.read();
                shared_data_.write(current + 1);
                response = "UPDATED: " + std::to_string(current + 1) + "\n";
            } 
            else {
                response = "ERROR: Invalid request\n";
            }

            send(client_fd, response.c_str(), response.size(), 0);
        } 
        catch (const std::exception& e) {
            std::cerr << "Connection handling error: " << e.what() << "\n";
        }
    }
};

int main() {
    try {
        ThreadSafeData shared_data;
        ConnectionHandler handler(shared_data);

        // Creates TCP socket, AF_INET = IPv4, SOCK_STREAM = TCP
        os_socket::Socket server(AF_INET, SOCK_STREAM, 0);
        server.bind(8080);
        server.listen(); // Default value is 10

        std::cout << "Server running on port 8080...\n";

        std::vector<std::thread> workers;
        std::atomic<bool> running{true};

        // Worker thread function
        auto worker = [&]() {
            while (running) {
                try {
                    auto [client_fd, client_ip] = server.accept();
                    std::cout << "New connection from: " << client_ip << "\n";
                    handler.handle(client_fd);
                } 
                catch (const std::exception& e) {
                    if (running) {
                        std::cerr << "Accept error: " << e.what() << "\n";
                    }
                }
            }
        };

        // Start worker threads
        // hardware_concurrency: Creates one thread per cpu core
        unsigned num_threads = std::thread::hardware_concurrency();
        for (unsigned i = 0; i < num_threads; ++i) {
            workers.emplace_back(worker);
        }

        // Wait for shutdown signal
        std::cout << "Press Enter to shutdown...\n";
        std::cin.get();
        running = false;

        // Close server socket to unblock accept calls
        server.~Socket();

        // Join all threads
        for (auto& t : workers) {
            if (t.joinable()) t.join();
        }

        return 0;
    } 
    catch (const std::exception& e) {
        std::cerr << "Server fatal error: " << e.what() << "\n";
        return 1;
    }
}   