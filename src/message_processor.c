#include "message_processor.h"
#include "debug_macros.h"
#include "auth.h"
#include "cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>

static void process_single_message(MessageProcessor* mp);
static char* create_response(const char* status, const char* content_type, 
                           const char* body, const char* connection);
static char* create_error_response(const char* status, const char* message);
static bool verify_auth(HttpRequest* request, ThreadSafeData* tsd);

void message_processor_init(MessageProcessor* mp, MessageQueue* queue, 
                          ThreadSafeData* data) {
    mp->queue = queue;
    mp->shared_data = data;
    mp->running = true;
}

void message_processor_start(MessageProcessor* mp) {
    while (mp->running) {
        process_single_message(mp);
    }
}

void message_processor_stop(MessageProcessor* mp) {
    mp->running = false;
}

static bool verify_auth(HttpRequest* request, ThreadSafeData* tsd) {
    // Skip auth for signup and login
    if ((strcmp(request->method, "POST") == 0 && 
        (strcmp(request->path, "/signup") == 0 || strcmp(request->path, "/login") == 0))) {
        return true;
    }
    
    // Check Authorization header
    for (size_t i = 0; i < request->header_count; i++) {
        if (strcasecmp(request->header_keys[i], "Authorization") == 0) {
            return auth_verify_token(request->header_values[i], tsd);
        }
    }
    return false;
}

static void process_single_message(MessageProcessor* mp) {
    int client_fd;
    char* raw_request;
    
    if (!message_queue_pop(mp->queue, &client_fd, &raw_request)) {
        DEBUG_PRINT("Queue pop failed or shutdown\n");
        return;
    }

    HttpParseResult parse_result = http_parse_request(raw_request);
    free(raw_request);
    
    char* response = NULL;
    
    if (!parse_result.success) {
        response = create_error_response("400 Bad Request", "Bad Request");
    } else {
        HttpRequest* request = &parse_result.request;
        
        // Verify authentication for protected routes
        if (!verify_auth(request, mp->shared_data)) {
            response = create_error_response("401 Unauthorized", "Authentication required");
        }
        else if (strcmp(request->method, "GET") == 0 && 
            strcmp(request->path, "/users") == 0) {
            char* text_data = tsd_read_text(mp->shared_data);
            const char* connection = "close";
            for (size_t i = 0; i < request->header_count; i++) {
                if (strcasecmp(request->header_keys[i], "Connection") == 0) {
                    connection = request->header_values[i];
                    break;
                }
            }
            
            response = create_response("200 OK", "text/plain", text_data ? text_data : "", connection);
            if (text_data) free(text_data);
        }
        else if (strcmp(request->method, "POST") == 0 && 
                 strcmp(request->path, "/users") == 0) {
            if (!request->body || !*request->body) {
                response = create_error_response("400 Bad Request", "Missing request body");
            } else {
                if (tsd_write_text(mp->shared_data, request->body)) {
                    response = create_response("201 Created", "text/plain", "Data saved successfully", "close");
                } else {
                    response = create_error_response("500 Internal Server Error", "Failed to save data");
                }
            }
        }
        else if (strcmp(request->method, "POST") == 0 && strcmp(request->path, "/signup") == 0) {
            cJSON* req_json = cJSON_Parse(request->body);
            if (!req_json) {
                response = create_error_response("400 Bad Request", "Invalid JSON");
            } else {
                cJSON* username_obj = cJSON_GetObjectItem(req_json, "username");
                cJSON* password_obj = cJSON_GetObjectItem(req_json, "password");
                
                char* username = username_obj ? username_obj->valuestring : NULL;
                char* password = password_obj ? password_obj->valuestring : NULL;
                
                if (username && password && auth_signup(mp->shared_data, username, password)) {
                    response = create_response("201 Created", "application/json", 
                                            "{\"status\":\"success\",\"message\":\"User created\"}", "close");
                } else {
                    response = create_error_response("400 Bad Request", "Signup failed - username may be taken");
                }
                cJSON_Delete(req_json);
            }
        }
        else if (strcmp(request->method, "POST") == 0 && strcmp(request->path, "/login") == 0) {
            cJSON* req_json = cJSON_Parse(request->body);
            if (!req_json) {
                response = create_error_response("400 Bad Request", "Invalid JSON");
            } else {
                cJSON* username_obj = cJSON_GetObjectItem(req_json, "username");
                cJSON* password_obj = cJSON_GetObjectItem(req_json, "password");
                
                char* username = username_obj ? username_obj->valuestring : NULL;
                char* password = password_obj ? password_obj->valuestring : NULL;
                
                char* token = auth_login(mp->shared_data, username, password);
                if (token) {
                    char response_body[256];
                    snprintf(response_body, sizeof(response_body), 
                            "{\"status\":\"success\",\"token\":\"%s\"}", token);
                    response = create_response("200 OK", "application/json", response_body, "close");
                    free(token);
                } else {
                    response = create_error_response("401 Unauthorized", "Invalid credentials");
                }
                cJSON_Delete(req_json);
            }
        } else {
            response = create_error_response("404 Not Found", "Not Found");
        }
    }
    
    if (response) {
        if (send(client_fd, response, strlen(response), 0) == -1) {
            DEBUG_PRINT("Send failed: %s\n", strerror(errno));
        }
        free(response);
    }
    
    if (client_fd >= 0) {
        shutdown(client_fd, SHUT_RDWR);
        close(client_fd);
    }
    
    http_request_free(&parse_result.request);
}

static char* create_response(const char* status, const char* content_type, 
                           const char* body, const char* connection) {
    size_t length = snprintf(NULL, 0, 
        "HTTP/1.1 %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Connection: %s\r\n\r\n%s",
        status, content_type, strlen(body), connection, body);
    
    char* response = (char*)malloc(length + 1);
    if (!response) return NULL;
    
    sprintf(response,
        "HTTP/1.1 %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Connection: %s\r\n\r\n%s",
        status, content_type, strlen(body), connection, body);
    
    return response;
}

static char* create_error_response(const char* status, const char* message) {
    return create_response(status, "text/plain", message, "close");
}