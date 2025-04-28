#ifndef MESSAGE_QUEUE_H
#define MESSAGE_QUEUE_H

#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>

typedef struct {
    int client_fd;
    char* message;
} Message;

typedef struct {
    Message* messages;
    size_t capacity;
    size_t front;
    size_t rear;
    size_t size;
    
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    bool shutdown_flag;
} MessageQueue;

void message_queue_init(MessageQueue* mq, size_t capacity);

void message_queue_destroy(MessageQueue* mq);

bool message_queue_push(MessageQueue* mq, int client_fd, const char* message);

bool message_queue_pop(MessageQueue* mq, int* client_fd, char** message);

void message_queue_shutdown(MessageQueue* mq);

#endif // MESSAGE_QUEUE_H