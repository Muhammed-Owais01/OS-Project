#pragma once
#include "message_queue.hpp"

class ConnectionHandler {
    MessageQueue& queue_;
public:
    ConnectionHandler(MessageQueue& queue);
    void handle(int client_fd);
};