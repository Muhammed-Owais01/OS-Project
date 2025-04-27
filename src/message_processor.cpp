#include <string>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include "message_processor.hpp"
#include "http_parser.hpp"
using json = nlohmann::json;

MessageProcessor::MessageProcessor(MessageQueue& queue, ThreadSafeData& data) : queue_(queue), shared_data_(data) {}

void MessageProcessor::processMessages() {
    while (true) {
        try {
            auto msg_opt = queue_.pop();
            if (!msg_opt) break; // Queue was shutdown
            
            auto [client_fd, raw_request] = *msg_opt;
            std::string response;

            auto request_opt = HttpParser::parse(raw_request);
            if(!request_opt){
                response = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nContent-Length: 11\r\nConnection: close\r\n\r\nBad Request";
            }
            else {
                auto& request = *request_opt;

                if (request.method == "GET" && request.path == "/users") {
                    json data = shared_data_.read();
                    std::string body = data.dump(2);
                    response = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: " + 
                               std::to_string(body.size()) + "\r\nConnection: close\r\n\r\n" + body;
                } else if (request.method == "POST" && request.path == "/users") {
                    try {
                        json new_user = json::parse(request.body);
                        if (!new_user.contains("name") || !new_user.contains("email")) {
                            throw std::runtime_error("Missing name or email");
                        }

                        json data = shared_data_.read();
                        int new_id = data["users"].size() + 1;
                        new_user["id"] = new_id;
                        data["users"].push_back(new_user);
                        shared_data_.write(data);

                        std::string body = new_user.dump(2);
                        response = "HTTP/1.1 201 Created\r\nContent-Type: application/json\r\nContent-Length: " + 
                                   std::to_string(body.size()) + "\r\nConnection: close\r\n\r\n" + body;
                    } catch (const std::exception& e) {
                        std::string body = "Error: " + std::string(e.what());
                        response = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nContent-Length: " + 
                                   std::to_string(body.size()) + "\r\nConnection: close\r\n\r\n" + body;
                    }
                } else {
                    response = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\nContent-Length: 9\r\nConnection: close\r\n\r\nNot Found";
                }
            }

            if (send(client_fd, response.c_str(), response.size(), 0) == -1) {
                std::cerr << "Send error\n";
            }
            
            close(client_fd);
        } 
        catch (const std::exception& e) {
            std::cerr << "Processing error: " << e.what() << "\n";
        }
    }
} 