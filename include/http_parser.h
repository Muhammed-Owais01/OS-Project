#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#include <stdbool.h>
#include <stdlib.h>
// Ensure strdup is declared
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#include <string.h>

typedef struct {
    char* method;
    char* path;
    char** header_keys;
    char** header_values;
    size_t header_count;
    char* body;
} HttpRequest;

typedef struct {
    bool success;
    HttpRequest request;
} HttpParseResult;

void http_request_init(HttpRequest* req);

void http_request_free(HttpRequest* req);

HttpParseResult http_parse_request(const char* raw_request);

#endif // HTTP_PARSER_H