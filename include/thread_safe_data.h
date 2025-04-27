#ifndef THREAD_SAFE_DATA_H
#define THREAD_SAFE_DATA_H

#include <pthread.h>
#include <stdbool.h>
#include "cJSON.h"

typedef struct {
    cJSON* data;
    pthread_mutex_t mutex;
    const char* filename;
} ThreadSafeData;

void tsd_init(ThreadSafeData* tsd);
void tsd_destroy(ThreadSafeData* tsd);
cJSON* tsd_read(ThreadSafeData* tsd);
void tsd_write(ThreadSafeData* tsd, cJSON* value);

#endif // THREAD_SAFE_DATA_H