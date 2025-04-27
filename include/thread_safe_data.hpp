#pragma once
#include <mutex>
#include <nlohmann/json.hpp>
#include <string>

using json = nlohmann::json;

class ThreadSafeData {
private:
    json data_;
    mutable std::mutex mutex_;
    const std::string filename_ = "src/users.json";
    void loadFromFile();
    void saveToFile() const;
public:
    ThreadSafeData();
    json read() const;
    void write(const json& value);
};