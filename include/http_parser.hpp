#pragma once
#include <string>
#include <map>
#include <optional>

struct HttpRequest {
    std::string method;
    std::string path;
    std::map<std::string, std::string> headers;
    std::string body;
};

class HttpParser {
public:
    static std::optional<HttpRequest> parse(const std::string& raw_request);
};