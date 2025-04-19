#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <utility>
#include <atomic>
#include <optional>
#include <string>


class MessageQueue {
    std::queue<std::pair<int, std::string>> queue_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::atomic<bool> shutdown_flag{false};
public:
    void push(int client_fd, const std::string& message);
    std::optional<std::pair<int, std::string>> pop();
    
    void shutdown() {
        shutdown_flag = true;
        // Wake up any waiting threads
        cv_.notify_all();
    }
};