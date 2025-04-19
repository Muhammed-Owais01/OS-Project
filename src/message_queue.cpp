#include "message_queue.hpp"

void MessageQueue::push(int client_fd, const std::string &message) {
    std::unique_lock<std::mutex> lock(mutex_);
    queue_.emplace(client_fd, message);
    cv_.notify_one();
}

std::optional<std::pair<int, std::string>> MessageQueue::pop() {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock, [this] { return !queue_.empty() || shutdown_flag; });
    
    if (shutdown_flag) {
        return std::nullopt;
    }
    
    auto message = queue_.front();
    queue_.pop();
    return message;
}