#include "connection_handler.h"
#include "http_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <errno.h>

void connection_handler_init(ConnectionHandler* handler, MessageQueue* queue) {
    handler->queue = queue;
}

bool connection_handler_handle(ConnectionHandler* handler, int client_fd) {
    struct timeval timeout = {.tv_sec = 10, .tv_usec = 0};
    if (setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        fprintf(stderr, "Failed to set socket timeout: %s\n", strerror(errno));
        close(client_fd);
        return false;
    }

    char buffer[4096] = {0};
    ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

    if (bytes_read <= 0) {
        if (bytes_read == 0) {
            fprintf(stderr, "Client disconnected\n");
        } else {
            fprintf(stderr, "Receive error: %s\n", strerror(errno));
        }
        close(client_fd);
        return false;
    }

    buffer[bytes_read] = '\0';
    printf("Received request:\n%s\n---\n", buffer);

    HttpParseResult parse_result = http_parse_request(buffer);
    if (!parse_result.success) {
        const char* response = 
            "HTTP/1.1 400 Bad Request\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 11\r\n"
            "Connection: close\r\n\r\n"
            "Bad Request";
        
        send(client_fd, response, strlen(response), 0);
        close(client_fd);
        http_request_free(&parse_result.request);
        return false;
    }

    if (!message_queue_push(handler->queue, client_fd, buffer)) {
        fprintf(stderr, "Failed to push message to queue\n");
        close(client_fd);
        http_request_free(&parse_result.request);
        return false;
    }

    http_request_free(&parse_result.request);
    return true;
}