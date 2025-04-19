#pragma once
#include <mutex>

class ThreadSafeData {
private:
    int data_ = 0;
    mutable std::mutex mutex_;
public:
    int read() const;
    void write(int value);
};