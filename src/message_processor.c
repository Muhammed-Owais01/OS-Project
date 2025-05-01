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
        
        if (strcmp(request->method, "GET") == 0 && 
            strcmp(request->path, "/users") == 0) {
            cJSON* data = tsd_read(mp->shared_data);
            char* body = data ? cJSON_Print(data) : NULL;
            if (data) cJSON_Delete(data);
            
            const char* connection = "close";
            for (size_t i = 0; i < request->header_count; i++) {
                if (strcasecmp(request->header_keys[i], "Connection") == 0) {
                    connection = request->header_values[i];
                    break;
                }
            }
            
            response = create_response("200 OK", "application/json", body ? body : "{}", connection);
            if (body) free(body);
        }
        else if (strcmp(request->method, "POST") == 0 && 
                 strcmp(request->path, "/users") == 0) {
            
            if (!request->body || !*request->body) {
                response = create_error_response("400 Bad Request", "Missing request body");
            } else {
                cJSON* new_user = cJSON_Parse(request->body);
                
                if (!new_user) {
                    response = create_error_response("400 Bad Request", "Invalid JSON");
                } else {
                    cJSON* name = cJSON_GetObjectItem(new_user, "name");
                    cJSON* email = cJSON_GetObjectItem(new_user, "email");
                    
                    if (!name || !email) {
                        response = create_error_response("400 Bad Request", "Missing name or email");
                        cJSON_Delete(new_user);
                    } else {
                        cJSON* data = tsd_read(mp->shared_data);
                        
                        if (!data) {
                            response = create_error_response("500 Internal Server Error", "Data access failed");
                            cJSON_Delete(new_user);
                        } else {
                            cJSON* users = cJSON_GetObjectItem(data, "users");
                            
                            if (!users) {
                                response = create_error_response("500 Internal Server Error", "Invalid data structure");
                                cJSON_Delete(data);
                                cJSON_Delete(new_user);
                            } else {
                                int new_id = cJSON_GetArraySize(users) + 1;
                                
                                cJSON* new_user_copy = cJSON_Duplicate(new_user, 1);
                                if (!new_user_copy) {
                                    response = create_error_response("500 Internal Server Error", "Memory allocation failed");
                                } else {
                                    cJSON_AddNumberToObject(new_user_copy, "id", new_id);
                                    cJSON_AddItemToArray(users, new_user_copy);

                                    cJSON* data_to_write = cJSON_Duplicate(data, 1);
                                    if (data_to_write) {
                                        tsd_write(mp->shared_data, data_to_write);
                                        
                                        char* body = cJSON_Print(new_user_copy);
                                        response = create_response("201 Created", "application/json", 
                                                                 body ? body : "{}", "close");
                                        if (body) free(body);
                                    } else {
                                        response = create_error_response("500 Internal Server Error", "Data copy failed");
                                    }
                                }
                            
                                cJSON_Delete(data);
                                cJSON_Delete(new_user);
                            }
                        }
                    }
                }
            }
        }
        
        else if (strcmp(request->method, "POST") == 0 && strcmp(request->path, "/signup") == 0) {
            cJSON* req_json = cJSON_Parse(request->body);
            if (!req_json) {
                response = create_error_response("400 Bad Request", "Invalid JSON");
            } else {
                char* username = cJSON_GetStringValue(cJSON_GetObjectItem(req_json, "username"));
                char* password = cJSON_GetStringValue(cJSON_GetObjectItem(req_json, "password"));
                
                if (username && password && auth_signup(mp->shared_data, username, password)) {
                    response = create_response("201 Created", "application/json", 
                                            "{\"status\":\"success\"}", "close");
                } else {
                    response = create_error_response("400 Bad Request", "Signup failed");
                }
                cJSON_Delete(req_json);
            }
        }
        else if (strcmp(request->method, "POST") == 0 && strcmp(request->path, "/login") == 0) {
            cJSON* req_json = cJSON_Parse(request->body);
            if (!req_json) {
                response = create_error_response("400 Bad Request", "Invalid JSON");
            } else {
                char* username = cJSON_GetStringValue(cJSON_GetObjectItem(req_json, "username"));
                char* password = cJSON_GetStringValue(cJSON_GetObjectItem(req_json, "password"));
                
                if (username && password && auth_login(mp->shared_data, username, password)) {
                    response = create_response("200 OK", "application/json",
                                            "{\"status\":\"success\"}", "close");
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
    // Calculate needed buffer size
    size_t length = snprintf(NULL, 0, 
        "HTTP/1.1 %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Connection: %s\r\n\r\n%s",
        status, content_type, strlen(body), connection, body);
    
    // Allocate with explicit cast
    char* response = (char*)malloc(length + 1);
    if (!response) return NULL;
    
    // Format the response
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