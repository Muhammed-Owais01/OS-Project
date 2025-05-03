#include "message_queue.h"
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

void message_queue_init(MessageQueue* mq, size_t capacity) {
    mq->messages = (Message*)malloc(capacity * sizeof(Message));
    mq->capacity = capacity;
    mq->front = 0;
    mq->rear = 0;
    mq->size = 0;
    mq->shutdown_flag = false;
    pthread_mutex_init(&mq->mutex, NULL);
    pthread_cond_init(&mq->cond, NULL);
}

void message_queue_destroy(MessageQueue* mq) {
    pthread_mutex_lock(&mq->mutex);
    
    while (mq->size > 0) {
        free(mq->messages[mq->front].message);
        mq->front = (mq->front + 1) % mq->capacity;
        mq->size--;
    }
    
    free(mq->messages);
    pthread_mutex_unlock(&mq->mutex);
    pthread_mutex_destroy(&mq->mutex);
    pthread_cond_destroy(&mq->cond);
}

bool message_queue_push(MessageQueue* mq, int client_fd, const char* message) {
    pthread_mutex_lock(&mq->mutex);
    
    if (mq->shutdown_flag) {
        pthread_mutex_unlock(&mq->mutex);
        return false;
    }
    
    // Check if queue is full
    if (mq->size == mq->capacity) {
        pthread_mutex_unlock(&mq->mutex);
        return false;
    }
    
    // Copy the message
    char* msg_copy = strdup(message);
    if (!msg_copy) {
        pthread_mutex_unlock(&mq->mutex);
        return false;
    }
    
    // Add to queue
    mq->messages[mq->rear].client_fd = client_fd;
    mq->messages[mq->rear].message = msg_copy;
    mq->rear = (mq->rear + 1) % mq->capacity;
    mq->size++;
    
    pthread_cond_signal(&mq->cond);
    pthread_mutex_unlock(&mq->mutex);
    return true;
}

bool message_queue_pop(MessageQueue* mq, int* client_fd, char** message) {
    pthread_mutex_lock(&mq->mutex);
    
    while (mq->size == 0 && !mq->shutdown_flag) {
        pthread_cond_wait(&mq->cond, &mq->mutex);
    }
    
    if (mq->shutdown_flag) {
        pthread_mutex_unlock(&mq->mutex);
        return false;
    }
    
    // Get message from queue
    *client_fd = mq->messages[mq->front].client_fd;
    *message = mq->messages[mq->front].message;
    
    mq->front = (mq->front + 1) % mq->capacity;
    mq->size--;
    
    pthread_mutex_unlock(&mq->mutex);
    return true;
}

void message_queue_shutdown(MessageQueue* mq) {
    pthread_mutex_lock(&mq->mutex);
    mq->shutdown_flag = true;
    pthread_cond_broadcast(&mq->cond);
    pthread_mutex_unlock(&mq->mutex);
}