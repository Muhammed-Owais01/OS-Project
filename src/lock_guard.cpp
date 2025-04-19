#include "lock_guard.hpp"

LockGuard::LockGuard(std::mutex& mutex) : mutex_(mutex) {
    mutex_.lock();
}

LockGuard::~LockGuard() { mutex_.unlock(); }