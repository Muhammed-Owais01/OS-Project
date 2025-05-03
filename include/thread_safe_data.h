#ifndef THREAD_SAFE_DATA_H
#define THREAD_SAFE_DATA_H

#include <pthread.h>
#include "cJSON.h"
#include <stdbool.h>

typedef struct {
    char* filename;
    pthread_mutex_t mutex;
    cJSON* data;
} ThreadSafeData;

void tsd_init(ThreadSafeData* tsd);
void tsd_destroy(ThreadSafeData* tsd);
cJSON* tsd_read(ThreadSafeData* tsd);
bool tsd_write(ThreadSafeData* tsd, cJSON* value); 
#endif 