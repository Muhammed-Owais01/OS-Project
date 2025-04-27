#include "http_parser.h"
#include <string.h>
#include "cJSON.h"
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

#define MAX_HEADERS 50
#define INITIAL_BUF_SIZE 1024

static char* strtrim(char* str);
static int find_char(const char* str, char c);

void http_request_init(HttpRequest* req) {
    req->method = NULL;
    req->path = NULL;
    req->header_keys = NULL;
    req->header_values = NULL;
    req->header_count = 0;
    req->body = NULL;
}

void http_request_free(HttpRequest* req) {
    if (!req) return;
    
    // Add null checks and prevent double-free
    if (req->method) {
        free(req->method);
        req->method = NULL;
    }
    if (req->path) {
        free(req->path);
        req->path = NULL;
    }
    
    if (req->header_keys && req->header_values) {
        for (size_t i = 0; i < req->header_count; i++) {
            if (req->header_keys[i]) free(req->header_keys[i]);
            if (req->header_values[i]) free(req->header_values[i]);
        }
    }
    
    free(req->header_keys); req->header_keys = NULL;
    free(req->header_values); req->header_values = NULL;
    
    if (req->body) {
        free(req->body);
        req->body = NULL;
    }
    
    req->header_count = 0;
}

HttpParseResult http_parse_request(const char* raw_request) {
    HttpParseResult result;
    result.success = false;
    http_request_init(&result.request);
    
    printf("\n====== RAW REQUEST START ======\n");
    printf("%s", raw_request);
    printf("\n====== RAW REQUEST END ======\n");
    
    if (!raw_request || !*raw_request) {
        printf("Parser: Empty raw request\n");
        return result;
    }

    // Make a working copy
    char* request_copy = strdup(raw_request);
    if (!request_copy) {
        printf("Parser: strdup failed\n");
        return result;
    }

    // Find the header/body separator
    char* body_start = NULL;
    char* current = request_copy;
    
    // Look for \r\n\r\n or \n\n
    while (*current) {
        if (*current == '\n') {
            // Check for empty line
            if (*(current+1) == '\n' || 
                (*(current+1) == '\r' && *(current+2) == '\n')) {
                body_start = current + 
                    (*(current+1) == '\r' ? 2 : 1) + 1;
                printf("Parser: Found body at: %p ('%.20s')\n", 
                      body_start, body_start);
                break;
            }
        }
        current++;
    }

    if (!body_start) {
        printf("Parser: No empty line found between headers and body\n");
        free(request_copy);
        return result;
    }

    // Parse request line (first line)
    char* line_end = strchr(request_copy, '\n');
    if (!line_end) {
        printf("Parser: No newline in request\n");
        free(request_copy);
        return result;
    }
    *line_end = '\0';  // Terminate request line
    
    // Parse method, path, version
    char* method = strtok(request_copy, " ");
    char* path = method ? strtok(NULL, " ") : NULL;
    char* version = path ? strtok(NULL, " ") : NULL;
    
    if (!method || !path || !version) {
        printf("Parser: Invalid request line\n");
        free(request_copy);
        return result;
    }

    printf("Parser: Method='%s' Path='%s' Version='%s'\n", 
           method, path, version);

    result.request.method = strdup(method);
    result.request.path = strdup(path);

    // Allocate header arrays
    result.request.header_keys = (char**)malloc(MAX_HEADERS * sizeof(char*));
    result.request.header_values = (char**)malloc(MAX_HEADERS * sizeof(char*));
    
    if (!result.request.header_keys || !result.request.header_values) {
        printf("Parser: Header array allocation failed\n");
        free(request_copy);
        http_request_free(&result.request);
        return result;
    }

    // Parse headers (between request line and empty line)
    char* headers_start = line_end + 1;
    char* headers_end = body_start - 
        (*(body_start-2) == '\r' ? 2 : 1) - 1;
    *headers_end = '\0';

    printf("Parser: Headers section:\n%s\n", headers_start);

    char* header_line = strtok(headers_start, "\n");
    while (header_line && result.request.header_count < MAX_HEADERS) {
        // Trim \r if present
        size_t len = strlen(header_line);
        if (len > 0 && header_line[len-1] == '\r') {
            header_line[len-1] = '\0';
        }

        // Parse header
        char* colon = strchr(header_line, ':');
        if (colon) {
            *colon = '\0';
            char* key = strtrim(header_line);
            char* value = strtrim(colon + 1);
            
            if (key && value) {
                result.request.header_keys[result.request.header_count] = strdup(key);
                result.request.header_values[result.request.header_count] = strdup(value);
                if (result.request.header_keys[result.request.header_count] && 
                    result.request.header_values[result.request.header_count]) {
                    result.request.header_count++;
                }
            }
        }
        header_line = strtok(NULL, "\n");
    }

    // Store body
    if (*body_start) {
        result.request.body = strdup(body_start);
        printf("Parser: Body stored: '%.*s%s'\n", 
               50, result.request.body, 
               strlen(result.request.body) > 50 ? "..." : "");
    } else {
        printf("Parser: Empty body\n");
    }

    free(request_copy);
    result.success = true;
    printf("Parser: Parse successful. Headers: %zu, Body: %s\n",
           result.request.header_count,
           result.request.body ? "present" : "missing");
    
    return result;
}

static char* strtrim(char* str) {
    if (!str) return NULL;
    
    while (isspace((unsigned char)*str)) str++;
    
    if (*str == 0) return str;
    
    char* end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    
    end[1] = '\0';
    return str;
}

static int find_char(const char* str, char c) {
    const char* pos = strchr(str, c);
    return pos ? (int)(pos - str) : -1;
}