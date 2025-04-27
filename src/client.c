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
} Client;

void client_init(Client* client, const char* server_ip, int server_port) {
    client->server_ip = strdup(server_ip);
    client->server_port = server_port;
}

void client_cleanup(Client* client) {
    free(client->server_ip);
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

bool client_run_interactive(Client* client) {
    while (true) {
        printf("1. GET /users\n");
        printf("2. POST /users\n");
        printf("3. Exit\n");
        printf("Choice: ");

        int choice;
        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n'); // Clear input buffer
            printf("Invalid input\n");
            continue;
        }
        while (getchar() != '\n'); // Clear the newline

        switch (choice) {
            case 1: {
                char* response = client_send_request(client, "GET", "/users", NULL);
                if (response) {
                    printf("Server response:\n%s\n", response);
                    free(response);
                } else {
                    printf("Request failed\n");
                }
                break;
            }
            case 2: {
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
                    printf("Server response:\n%s\n", response);
                    free(response);
                } else {
                    printf("Request failed\n");
                }
                break;
            }
            case 3:
                return true;
            default:
                printf("Invalid choice\n");
        }
    }
}

int main() {
    Client client;
    client_init(&client, "127.0.0.1", 8080);

    if (!client_run_interactive(&client)) {
        fprintf(stderr, "Client error occurred\n");
        client_cleanup(&client);
        return 1;
    }

    client_cleanup(&client);
    return 0;
}