#ifndef THREAD_SAFE_DATA_H
#define THREAD_SAFE_DATA_H

#include <pthread.h>
#include "cJSON.h"
#include <stdbool.h>

typedef struct {
    char* auth_filename;
    char* data_filename;
    pthread_mutex_t mutex;
    cJSON* auth_data;    // For storing authentication data (username/password)
} ThreadSafeData;

void tsd_init(ThreadSafeData* tsd);
void tsd_destroy(ThreadSafeData* tsd);
cJSON* tsd_read_auth(ThreadSafeData* tsd);
bool tsd_write_auth(ThreadSafeData* tsd, cJSON* value);
bool save_to_file_unlocked(ThreadSafeData* tsd);

// Text data functions
char* tsd_read_text(ThreadSafeData* tsd);
bool tsd_write_text(ThreadSafeData* tsd, const char* text);

#endif 