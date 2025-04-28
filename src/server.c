#include "socket.h"
#include "message_queue.h"
#include "thread_safe_data.h"
#include "connection_handler.h"
#include "message_processor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

#define MAX_THREADS 32

typedef struct {
    ConnectionHandler* handler;
    Socket* server;
} ThreadArgs;

ThreadSafeData shared_data;
bool running = true;
pthread_mutex_t running_mutex = PTHREAD_MUTEX_INITIALIZER;

bool get_running_status() {
    pthread_mutex_lock(&running_mutex);
    bool status = running;
    pthread_mutex_unlock(&running_mutex);
    return status;
}

void set_running_status(bool status) {
    pthread_mutex_lock(&running_mutex);
    running = status;
    pthread_mutex_unlock(&running_mutex);
}

void* worker_thread(void* arg) {
    ThreadArgs* args = (ThreadArgs*)arg;
    ConnectionHandler* handler = args->handler;
    Socket* server = args->server;
    
    while (get_running_status()) {
        int client_fd;
        char client_ip[INET_ADDRSTRLEN];
        
        if (socket_accept(server, &client_fd, client_ip) != 0) {
            if (get_running_status()) {
                fprintf(stderr, "Accept error: %s\n", strerror(errno));
            }
            continue;
        }
        
        printf("New connection from: %s\n", client_ip);
        connection_handler_handle(handler, client_fd);
    }
    return NULL;
}

void* processor_thread(void* arg) {
    MessageProcessor* processor = (MessageProcessor*)arg;
    
    while (get_running_status()) {
        message_processor_start(processor);
    }
    return NULL;
}

int main() {
    MessageQueue message_queue;
    Socket server;
    ConnectionHandler handler;
    MessageProcessor processor;
    
    pthread_t workers[MAX_THREADS];
    pthread_t processors[MAX_THREADS/2];
    unsigned num_threads = sysconf(_SC_NPROCESSORS_ONLN);
    
    // Initialize components
    if (socket_init(&server, AF_INET, SOCK_STREAM, 0) != 0) {
        fprintf(stderr, "Socket initialization failed\n");
        return 1;
    }
    
    if (socket_bind(&server, 8080, "0.0.0.0") != 0) {
        fprintf(stderr, "Bind failed\n");
        socket_destroy(&server);
        return 1;
    }
    
    if (socket_listen(&server, 10) != 0) {
        fprintf(stderr, "Listen failed\n");
        socket_destroy(&server);
        return 1;
    }
    
    message_queue_init(&message_queue, 1000);
    tsd_init(&shared_data);
    connection_handler_init(&handler, &message_queue);
    message_processor_init(&processor, &message_queue, &shared_data);
    
    printf("Server running on port 8080...\n");
    
    // Create worker threads
    ThreadArgs worker_args = {&handler, &server};
    for (unsigned i = 0; i < num_threads; ++i) {
        if (pthread_create(&workers[i], NULL, worker_thread, &worker_args) != 0) {
            perror("Failed to create worker thread");
            set_running_status(false);
            break;
        }
    }
    
    // Create processor threads
    for (unsigned i = 0; i < num_threads/2; ++i) {
        if (pthread_create(&processors[i], NULL, processor_thread, &processor) != 0) {
            perror("Failed to create processor thread");
            set_running_status(false);
            break;
        }
    }
    
    printf("Press Enter to shutdown...\n");
    getchar();
    set_running_status(false);
    
    // Cleanup
    socket_destroy(&server);
    message_queue_shutdown(&message_queue);
    
    // Wait for threads
    for (unsigned i = 0; i < num_threads; ++i) {
        if (workers[i]) pthread_join(workers[i], NULL);
    }
    
    for (unsigned i = 0; i < num_threads/2; ++i) {
        if (processors[i]) pthread_join(processors[i], NULL);
    }
    
    // Final cleanup
    pthread_mutex_destroy(&running_mutex);
    message_queue_destroy(&message_queue);
    tsd_destroy(&shared_data);
    
    return 0;
}