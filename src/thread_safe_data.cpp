#include "thread_safe_data.hpp"
#include "lock_guard.hpp"
#include <fstream>
#include <stdexcept>
#include <iostream>
#include <cerrno>
#include <cstring>

ThreadSafeData::ThreadSafeData() {
    loadFromFile();
}

void ThreadSafeData::loadFromFile() {
    LockGuard lock(mutex_);
    std::cerr << "Loading from file: " << filename_ << "\n";
    std::ifstream file(filename_);
    if (!file.is_open()) {
        std::cerr << "File not found, initializing empty data\n";
        data_ = json{{"users", json::array()}};
        saveToFile();
        return;
    }
    try {
        file >> data_;
        std::cerr << "File loaded successfully\n";
    } catch (const json::exception& e) {
        std::cerr << "JSON parse error: " << e.what() << "\n";
        throw std::runtime_error("Failed to parse JSON: " + std::string(e.what()));
    }
}

void ThreadSafeData::saveToFile() const {
    std::cerr << "Acquiring lock for file write...\n";
    LockGuard lock(mutex_);
    std::cerr << "Lock acquired, opening file...\n";
    
    std::ofstream file(filename_, std::ios::trunc);
    if (!file.is_open()) {
        std::cerr << "CRITICAL: Failed to open file: " << strerror(errno) << "\n";
        throw std::runtime_error("File open failed");
    }
    std::cerr << "File opened successfully, writing data...\n";

    try {
        file << data_.dump(2);
        file.flush(); // Force write to disk
        if (!file.good()) {
            std::cerr << "CRITICAL: Write failed: " << strerror(errno) << "\n";
            throw std::runtime_error("File write failed");
        }
        std::cerr << "Data written successfully, closing file...\n";
        file.close();
    } catch (const std::exception& e) {
        std::cerr << "CRITICAL: Exception during save: " << e.what() << "\n";
        throw;
    }
    std::cerr << "File operation completed successfully.\n";
}

json ThreadSafeData::read() const {
    LockGuard lock(mutex_);
    return data_;
}

void ThreadSafeData::write(const json& value) {
    {
        LockGuard lock(mutex_);
        data_ = value;
    } // Lock released here
    saveToFile(); // Now safe to acquire lock again
}
