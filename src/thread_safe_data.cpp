#include "thread_safe_data.hpp"
#include "lock_guard.hpp"
#include <fstream>
#include <stdexcept>
ThreadSafeData::ThreadSafeData(){
    loadFromFile();
}
void ThreadSafeData::loadFromFile(){
    LockGuard lock(mutex_);
    std::ifstream file(filename_);
    if(!file.is_open()){
        data_ = json{{"users", json::array()}};
        saveToFile();
        return;
    }
    try{
        file>>data_;
    } catch (const json::exception& e){
        throw std::runtime_error("Failed to parse JSON: " + std::string(e.what()));
    }
}
void ThreadSafeData::saveToFile() const{
    LockGuard lock(mutex_);
    std::ofstream file(filename_);
    if(!file.is_open()){
        throw std::runtime_error("Failed to open file for writing");
    }
    file << data_.dump(2);
}

json ThreadSafeData::read() const {
    LockGuard lock(mutex_);
    return data_;
}

void ThreadSafeData::write(const json& value) {
    LockGuard lock(mutex_);
    data_ = value;
    saveToFile();
}