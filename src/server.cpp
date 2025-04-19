#include "connection_handler.hpp"
#include "thread_safe_data.hpp"
#include "message_processor.hpp"
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

ThreadSafeData shared_data;

int main() {
    try {
        MessageQueue message_queue;
        std::atomic<bool> running{true};

        // Creates TCP socket
        os_socket::Socket server(AF_INET, SOCK_STREAM, 0);
        server.bind(8080);
        server.listen();
        ConnectionHandler handler(message_queue);
        
        std::cout << "Server running on port 8080...\n";
        
        std::vector<std::thread> workers;
        std::vector<std::thread> processors;

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
        
        // Processor thread function
        auto processor_func = [&]() {
            MessageProcessor processor(message_queue, shared_data);
            while (running) {
                processor.processMessages();
            }
        };

        // Start worker threads
        unsigned num_threads = std::thread::hardware_concurrency();
        for (unsigned i = 0; i < num_threads; ++i) {
            workers.emplace_back(worker);
        }
        
        // Start processor threads (use half the cores for processors)
        for (unsigned i = 0; i < num_threads/2; ++i) {
            processors.emplace_back(processor_func);
        }

        // Wait for shutdown signal
        std::cout << "Press Enter to shutdown...\n";
        std::cin.get();
        running = false;

        // Close server socket to unblock accept calls
        server.~Socket();

        // Wake up all processor threads
        message_queue.shutdown();

        // Join all threads
        for (auto& t : workers) {
            if (t.joinable()) t.join();
        }
        
        for (auto& t : processors) {
            if (t.joinable()) t.join();
        }

        return 0;
    } 
    catch (const std::exception& e) {
        std::cerr << "Server fatal error: " << e.what() << "\n";
        return 1;
    }
}