#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "socket.h"
#include <stdbool.h>
#include "cJSON.h"

typedef struct {
    char* server_ip;
    int server_port;
    char* auth_token;  // Stores session token after login
} Client;

void client_init(Client* client, const char* server_ip, int server_port) {
    client->server_ip = strdup(server_ip);
    client->server_port = server_port;
    client->auth_token = NULL;
}

void client_cleanup(Client* client) {
    free(client->server_ip);
    if (client->auth_token) {
        free(client->auth_token);
    }
}

char* client_send_request(Client* client, const char* method, const char* path, const char* body) {
    Socket socket;
    if (socket_init(&socket, AF_INET, SOCK_STREAM, 0) != 0) {
        return NULL;
    }

    socket_set_receive_timeout(&socket, 30);

    if (socket_connect(&socket, client->server_ip, client->server_port) != 0) {
        socket_destroy(&socket);
        return NULL;
    }

    // Build request
    char request[4096] = {0};
    int length = snprintf(request, sizeof(request),
        "%s %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Content-Type: application/json\r\n",
        method, path, client->server_ip);

    // Add auth token if available
    if (client->auth_token) {
        length += snprintf(request + length, sizeof(request) - length,
            "Authorization: Bearer %s\r\n",
            client->auth_token);
    }

    if (body && body[0]) {
        length += snprintf(request + length, sizeof(request) - length,
            "Content-Length: %zu\r\n",
            strlen(body));
    }

    strcat(request, "Connection: close\r\n\r\n");

    if (body && body[0]) {
        strcat(request, body);
    }

    printf("Sending request:\n%s\n---\n", request);

    // Send request
    ssize_t sent = send(socket.sockfd, request, strlen(request), 0);
    if (sent <= 0) {
        socket_destroy(&socket);
        return NULL;
    }

    // Receive response
    char* response = (char*)malloc(4096);
    if (!response) {
        socket_destroy(&socket);
        return NULL;
    }

    ssize_t received = recv(socket.sockfd, response, 4095, 0);
    if (received <= 0) {
        free(response);
        socket_destroy(&socket);
        return NULL;
    }

    response[received] = '\0';
    socket_destroy(&socket);
    return response;
}

void print_help() {
    printf("\nAvailable commands:\n");
    printf("1. GET /users - List all users (requires login)\n");
    printf("2. POST /users - Create new user (requires login)\n");
    printf("3. Sign Up - Register new account\n");
    printf("4. Login - Authenticate and get token\n");
    printf("5. Help - Show this help message\n");
    printf("6. Exit - Quit the program\n");
}

bool client_run_interactive(Client* client) {
    print_help();
    
    while (true) {
        printf("\nEnter command (5 for help): ");
        
        char input[10];
        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }
        
        int choice;
        if (sscanf(input, "%d", &choice) != 1) {
            printf("Invalid input. Please enter a number.\n");
            continue;
        }

        switch (choice) {
            case 1: { // GET /users
                if (!client->auth_token) {
                    printf("Error: You need to login first (use command 4)\n");
                    break;
                }
                
                char* response = client_send_request(client, "GET", "/users", NULL);
                if (response) {
                    printf("\nServer response:\n%s\n", response);
                    
                    // Pretty-print JSON response if possible
                    cJSON* json = cJSON_Parse(response);
                    if (json) {
                        char* pretty = cJSON_Print(json);
                        if (pretty) {
                            printf("Formatted response:\n%s\n", pretty);
                            free(pretty);
                        }
                        cJSON_Delete(json);
                    }
                    free(response);
                } else {
                    printf("Request failed\n");
                }
                break;
            }
            case 2: { // POST /users
                if (!client->auth_token) {
                    printf("Error: You need to login first (use command 4)\n");
                    break;
                }
                
                char name[256], email[256];
                printf("Enter name: ");
                fgets(name, sizeof(name), stdin);
                name[strcspn(name, "\n")] = '\0';

                printf("Enter email: ");
                fgets(email, sizeof(email), stdin);
                email[strcspn(email, "\n")] = '\0';

                cJSON* user = cJSON_CreateObject();
                cJSON_AddStringToObject(user, "name", name);
                cJSON_AddStringToObject(user, "email", email);
                char* body = cJSON_PrintUnformatted(user);
                cJSON_Delete(user);

                char* response = client_send_request(client, "POST", "/users", body);
                free(body);

                if (response) {
                    printf("\nServer response:\n%s\n", response);
                    free(response);
                } else {
                    printf("Request failed\n");
                }
                break;
            }
            case 3: { // Sign Up
                char username[256], password[256];
                printf("Enter username: ");
                fgets(username, sizeof(username), stdin);
                username[strcspn(username, "\n")] = '\0';

                printf("Enter password: ");
                fgets(password, sizeof(password), stdin);
                password[strcspn(password, "\n")] = '\0';

                cJSON* req = cJSON_CreateObject();
                cJSON_AddStringToObject(req, "username", username);
                cJSON_AddStringToObject(req, "password", password);
                char* body = cJSON_PrintUnformatted(req);
                cJSON_Delete(req);

                char* response = client_send_request(client, "POST", "/signup", body);
                free(body);

                if (response) {
                    printf("\nServer response:\n%s\n", response);
                    free(response);
                } else {
                    printf("Signup failed\n");
                }
                break;
            }
            case 4: { // Login
                char username[256], password[256];
                printf("Enter username: ");
                fgets(username, sizeof(username), stdin);
                username[strcspn(username, "\n")] = '\0';

                printf("Enter password: ");
                fgets(password, sizeof(password), stdin);
                password[strcspn(password, "\n")] = '\0';

                cJSON* req = cJSON_CreateObject();
                cJSON_AddStringToObject(req, "username", username);
                cJSON_AddStringToObject(req, "password", password);
                char* body = cJSON_PrintUnformatted(req);
                cJSON_Delete(req);

                char* response = client_send_request(client, "POST", "/login", body);
                free(body);

                if (response) {
                    printf("\nServer response:\n%s\n", response);
                    
                    // Parse token from response
                    cJSON* json = cJSON_Parse(response);
                    if (json) {
                        cJSON* token = cJSON_GetObjectItem(json, "token");
                        if (token && token->valuestring) {
                            if (client->auth_token) {
                                free(client->auth_token);
                            }
                            client->auth_token = strdup(token->valuestring);
                            printf("Successfully logged in! Token saved.\n");
                        } else {
                            printf("Login failed - no token received\n");
                        }
                        cJSON_Delete(json);
                    }
                    free(response);
                } else {
                    printf("Login failed\n");
                }
                break;
            }
            case 5: // Help
                print_help();
                break;
            case 6: // Exit
                return true;
            default:
                printf("Invalid command. Enter 5 for help.\n");
        }
    }
    return true;
}

int main() {
    Client client;
    client_init(&client, "127.0.0.1", 8080);

    printf("Simple HTTP Client\n");
    printf("Server: %s:%d\n", client.server_ip, client.server_port);
    
    if (!client_run_interactive(&client)) {
        fprintf(stderr, "Client error occurred\n");
    }

    client_cleanup(&client);
    return 0;
}