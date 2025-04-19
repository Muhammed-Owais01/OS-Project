#include "thread_safe_data.hpp"
#include "lock_guard.hpp"

int ThreadSafeData::read() const {
    LockGuard lock(mutex_);
    return data_;
}

void ThreadSafeData::write(int value) {
    LockGuard lock(mutex_);
    data_ = value;
}