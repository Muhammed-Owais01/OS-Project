#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include "cJSON.h"
#include "debug_macros.h"

#define SERVER_HOST "127.0.0.1:8080"

void print_help() {
    printf("\nAvailable commands:\n");
    printf("1. GET /users - Read text data (requires login)\n");
    printf("2. POST /users - Write text data (requires login)\n");
    printf("3. Sign Up - Register new account\n");
    printf("4. Login - Authenticate and get token\n");
    printf("5. Help - Show this help message\n");
    printf("6. Exit - Quit the program\n\n");
}

int connect_to_server() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        printf("Error creating socket: %s\n", strerror(errno));
        return -1;
    }
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        printf("Error connecting to server: %s\n", strerror(errno));
        close(sockfd);
        return -1;
    }
    
    return sockfd;
}

void handle_get_users(int server_fd, const char* token) {
    char request[1024];
    snprintf(request, sizeof(request),
        "GET /users HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Content-Type: text/plain\r\n"
        "Authorization: Bearer %s\r\n"
        "Connection: close\r\n\r\n",
        SERVER_HOST, token);
    
    printf("Sending request:\n%s---\n\n", request);
    
    if (send(server_fd, request, strlen(request), 0) == -1) {
        printf("Error sending request: %s\n", strerror(errno));
        return;
    }
    
    char response[4096];
    int bytes_received = recv(server_fd, response, sizeof(response) - 1, 0);
    if (bytes_received > 0) {
        response[bytes_received] = '\0';
        printf("Server response:\n%s\n", response);
    }
}

void handle_post_users(int server_fd, const char* token) {
    char text_data[1024];
    printf("Enter text to save: ");
    if (fgets(text_data, sizeof(text_data), stdin) == NULL) {
        printf("Error reading input\n");
        return;
    }
    
    // Remove newline if present
    text_data[strcspn(text_data, "\n")] = 0;
    
    char request[2048];
    snprintf(request, sizeof(request),
        "POST /users HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Content-Type: text/plain\r\n"
        "Authorization: Bearer %s\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n\r\n"
        "%s",
        SERVER_HOST, token, strlen(text_data), text_data);
    
    printf("Sending request:\n%s---\n\n", request);
    
    if (send(server_fd, request, strlen(request), 0) == -1) {
        printf("Error sending request: %s\n", strerror(errno));
        return;
    }
    
    char response[4096];
    int bytes_received = recv(server_fd, response, sizeof(response) - 1, 0);
    if (bytes_received > 0) {
        response[bytes_received] = '\0';
        printf("Server response:\n%s\n", response);
    }
}

void handle_signup(int server_fd) {
    char username[128], password[128];
    printf("Enter username: ");
    if (fgets(username, sizeof(username), stdin) == NULL) return;
    username[strcspn(username, "\n")] = 0;
    
    printf("Enter password: ");
    if (fgets(password, sizeof(password), stdin) == NULL) return;
    password[strcspn(password, "\n")] = 0;
    
    cJSON* json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "username", username);
    cJSON_AddStringToObject(json, "password", password);
    char* json_str = cJSON_Print(json);
    cJSON_Delete(json);
    
    char request[2048];
    snprintf(request, sizeof(request),
        "POST /signup HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n\r\n"
        "%s",
        SERVER_HOST, strlen(json_str), json_str);
    
    free(json_str);
    
    printf("Sending request:\n%s---\n\n", request);
    
    if (send(server_fd, request, strlen(request), 0) == -1) {
        printf("Error sending request: %s\n", strerror(errno));
        return;
    }
    
    char response[4096];
    int bytes_received = recv(server_fd, response, sizeof(response) - 1, 0);
    if (bytes_received > 0) {
        response[bytes_received] = '\0';
        printf("Server response:\n%s\n", response);
    }
}

char* handle_login(int server_fd) {
    char username[128], password[128];
    printf("Enter username: ");
    if (fgets(username, sizeof(username), stdin) == NULL) return NULL;
    username[strcspn(username, "\n")] = 0;
    
    printf("Enter password: ");
    if (fgets(password, sizeof(password), stdin) == NULL) return NULL;
    password[strcspn(password, "\n")] = 0;
    
    cJSON* json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "username", username);
    cJSON_AddStringToObject(json, "password", password);
    char* json_str = cJSON_Print(json);
    cJSON_Delete(json);
    
    char request[2048];
    snprintf(request, sizeof(request),
        "POST /login HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n\r\n"
        "%s",
        SERVER_HOST, strlen(json_str), json_str);
    
    free(json_str);
    
    printf("Sending request:\n%s---\n\n", request);
    
    if (send(server_fd, request, strlen(request), 0) == -1) {
        printf("Error sending request: %s\n", strerror(errno));
        return NULL;
    }
    
    char response[4096];
    int bytes_received = recv(server_fd, response, sizeof(response) - 1, 0);
    if (bytes_received > 0) {
        response[bytes_received] = '\0';
        printf("Server response:\n%s\n", response);
        
        // Parse response to get token
        cJSON* json = cJSON_Parse(strstr(response, "\r\n\r\n") + 4);
        if (json) {
            cJSON* token_obj = cJSON_GetObjectItem(json, "token");
            if (token_obj && token_obj->valuestring) {
                char* token = strdup(token_obj->valuestring);
                cJSON_Delete(json);
                return token;
            }
            cJSON_Delete(json);
        }
    }
    return NULL;
}

int main() {
    char* token = NULL;
    print_help();
    
    while (1) {
        printf("\nEnter command (1-6): ");
        int choice;
        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n'); // Clear input buffer
            printf("Invalid input. Please enter a number between 1 and 6.\n");
            continue;
        }
        while (getchar() != '\n'); // Clear input buffer
        
        if (choice == 6) {
            printf("Exiting...\n");
            free(token);
            break;
        }
        
        if (choice == 5) {
            print_help();
            continue;
        }
        
        int server_fd = connect_to_server();
        if (server_fd == -1) {
            printf("Failed to connect to server\n");
            continue;
        }
        
        switch (choice) {
            case 1:
                if (!token) {
                    printf("Please login first\n");
                    close(server_fd);
                    continue;
                }
                handle_get_users(server_fd, token);
                break;
            case 2:
                if (!token) {
                    printf("Please login first\n");
                    close(server_fd);
                    continue;
                }
                handle_post_users(server_fd, token);
                break;
            case 3:
                handle_signup(server_fd);
                break;
            case 4: {
                char* new_token = handle_login(server_fd);
                if (new_token) {
                    free(token);
                    token = new_token;
                }
                break;
            }
            default:
                printf("Invalid choice\n");
                break;
        }
        
        close(server_fd);
    }
    
    return 0;
}