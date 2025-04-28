#ifndef CONNECTION_HANDLER_H
#define CONNECTION_HANDLER_H

#include "message_queue.h"
#include <stdbool.h>

typedef struct {
    MessageQueue* queue;
} ConnectionHandler;

void connection_handler_init(ConnectionHandler* handler, MessageQueue* queue);
bool connection_handler_handle(ConnectionHandler* handler, int client_fd);

#endif // CONNECTION_HANDLER_H