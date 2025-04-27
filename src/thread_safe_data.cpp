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
    LockGuard lock(mutex_);
    std::cerr << "Attempting to open file for writing: " << filename_ << "\n";
    std::ofstream file(filename_);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename_ << " (errno: " << strerror(errno) << ")\n";
        throw std::runtime_error("Failed to open file for writing: " + filename_ + " (errno: " + std::string(strerror(errno)) + ")");
    }
    std::cerr << "Writing JSON to file\n";
    try {
        file << data_.dump(2);
        if (file.fail()) {
            std::cerr << "File stream error during writing (errno: " << strerror(errno) << ")\n";
            throw std::runtime_error("File stream error during writing: " + std::string(strerror(errno)));
        }
        file.close();
        if (file.fail()) {
            std::cerr << "File stream error after closing (errno: " << strerror(errno) << ")\n";
            throw std::runtime_error("File stream error after closing: " + std::string(strerror(errno)));
        }
        std::cerr << "File write complete\n";
    } catch (const std::exception& e) {
        std::cerr << "File write error: " << e.what() << "\n";
        throw std::runtime_error("Failed to write JSON to file: " + std::string(e.what()));
    }
}

json ThreadSafeData::read() const {
    LockGuard lock(mutex_);
    return data_;
}

void ThreadSafeData::write(const json& value) {
    LockGuard lock(mutex_);
    std::cerr << "Writing new data to shared storage\n";
    data_ = value;
    saveToFile();
}