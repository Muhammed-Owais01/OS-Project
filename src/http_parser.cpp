#include "http_parser.hpp"
#include <sstream>
#include <algorithm>

std::optional<HttpRequest> HttpParser::parse(const std::string& raw_request) {
    HttpRequest request;
    std::istringstream stream(raw_request);
    std::string line;

    // Parse request line (e.g., GET /users HTTP/1.1)
    if (!std::getline(stream, line)) {
        return std::nullopt;
    }

    std::istringstream line_stream(line);
    std::string version;
    line_stream >> request.method >> request.path >> version;
    if (request.method.empty() || request.path.empty() || version.empty()) {
        return std::nullopt;
    }

    // Parse headers
    while (std::getline(stream, line) && line != "\r") {
        if (line.empty()) continue;
        auto colon_pos = line.find(':');
        if (colon_pos == std::string::npos) continue;

        std::string key = line.substr(0, colon_pos);
        std::string value = line.substr(colon_pos + 1);
        
        // Trim whitespace
        key.erase(std::remove_if(key.begin(), key.end(), ::isspace), key.end());
        value.erase(std::remove_if(value.begin(), key.end(), ::isspace), value.end());
        
        request.headers[key] = value;
    }

    // Parse body (if any)
    std::string body;
    while (std::getline(stream, line)) {
        body += line + "\n";
    }
    if (!body.empty() && body.back() == '\n') {
        body.pop_back(); // Remove trailing newline
    }
    request.body = body;

    return request;
}