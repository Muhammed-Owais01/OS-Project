#pragma once
#include <mutex>

class LockGuard {
private:
    std::mutex& mutex_;
public:
    explicit LockGuard(std::mutex& mutex);
    ~LockGuard();
};