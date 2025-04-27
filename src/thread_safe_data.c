// thread_safe_data.c
#include "thread_safe_data.h"
#include "debug_macros.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#define DEFAULT_FILENAME "src/users.json"

static void load_from_file(ThreadSafeData* tsd);
static void save_to_file_unlocked(ThreadSafeData* tsd);

void tsd_init(ThreadSafeData* tsd) {
    tsd->filename = DEFAULT_FILENAME;
    pthread_mutex_init(&tsd->mutex, NULL);
    tsd->data = NULL;
    load_from_file(tsd);
}

void tsd_destroy(ThreadSafeData* tsd) {
    pthread_mutex_lock(&tsd->mutex);
    if (tsd->data) {
        cJSON_Delete(tsd->data);
        tsd->data = NULL;
    }
    pthread_mutex_unlock(&tsd->mutex);
    pthread_mutex_destroy(&tsd->mutex);
}

static void load_from_file(ThreadSafeData* tsd) {
    pthread_mutex_lock(&tsd->mutex);
    DEBUG_PRINT("Loading from file: %s\n", tsd->filename);
    
    FILE* file = fopen(tsd->filename, "r");
    if (!file) {
        DEBUG_PRINT("File not found, initializing empty data\n");
        tsd->data = cJSON_CreateObject();
        if (tsd->data) {
            cJSON_AddItemToObject(tsd->data, "users", cJSON_CreateArray());
        }
        save_to_file_unlocked(tsd); 
        pthread_mutex_unlock(&tsd->mutex);
        return;
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (length <= 0) {
        DEBUG_PRINT("Empty or invalid file\n");
        fclose(file);
        pthread_mutex_unlock(&tsd->mutex);
        return;
    }

    char* buffer = (char*)malloc(length + 1);
    if (!buffer) {
        DEBUG_PRINT("Memory allocation failed\n");
        fclose(file);
        pthread_mutex_unlock(&tsd->mutex);
        return;
    }

    size_t read = fread(buffer, 1, length, file);
    fclose(file);
    
    if (read != (size_t)length) {
        DEBUG_PRINT("File read incomplete\n");
        free(buffer);
        pthread_mutex_unlock(&tsd->mutex);
        return;
    }

    buffer[length] = '\0';
    tsd->data = cJSON_Parse(buffer);
    free(buffer);
    
    if (!tsd->data) {
        const char* error_ptr = cJSON_GetErrorPtr();
        DEBUG_PRINT("JSON parse error at: %s\n", error_ptr ? error_ptr : "unknown");
        tsd->data = cJSON_CreateObject();
        if (tsd->data) {
            cJSON_AddItemToObject(tsd->data, "users", cJSON_CreateArray());
        }
    }
    
    DEBUG_PRINT("File loaded successfully\n");
    pthread_mutex_unlock(&tsd->mutex);
}

static void save_to_file_unlocked(ThreadSafeData* tsd) {
    DEBUG_ENTER();
    DEBUG_PTR(tsd);
    
    if (!tsd->data) {
        DEBUG_PRINT("No data to save\n");
        return;
    }

    DEBUG_PTR(tsd->data);
    
    char temp_filename[256];
    snprintf(temp_filename, sizeof(temp_filename), "%s.tmp", tsd->filename);
    
    FILE* file = fopen(temp_filename, "w");
    if (!file) {
        DEBUG_PRINT("Failed to open temp file: %s\n", strerror(errno));
        return;
    }

    char* json_str = cJSON_Print(tsd->data);
    if (!json_str) {
        DEBUG_PRINT("JSON print failed\n");
        fclose(file);
        remove(temp_filename);
        return;
    }

    size_t len = strlen(json_str);
    DEBUG_PRINT("Writing %zu bytes\n", len);
    
    size_t written = fwrite(json_str, 1, len, file);
    free(json_str);
    
    if (written != len) {
        DEBUG_PRINT("Write incomplete: %zu/%zu\n", written, len);
        fclose(file);
        remove(temp_filename);
        return;
    }

    if (fflush(file) != 0 || fsync(fileno(file)) != 0) {
        DEBUG_PRINT("Flush/sync failed: %s\n", strerror(errno));
        fclose(file);
        remove(temp_filename);
        return;
    }

    fclose(file);
    
    if (rename(temp_filename, tsd->filename) != 0) {
        DEBUG_PRINT("Rename failed: %s\n", strerror(errno));
        remove(temp_filename);
        return;
    }
    
    DEBUG_PRINT("File save completed\n");
    DEBUG_EXIT();
}

cJSON* tsd_read(ThreadSafeData* tsd) {
    pthread_mutex_lock(&tsd->mutex);
    cJSON* copy = tsd->data ? cJSON_Duplicate(tsd->data, 1) : NULL;
    pthread_mutex_unlock(&tsd->mutex);
    return copy;
}

void tsd_write(ThreadSafeData* tsd, cJSON* value) {
    pthread_mutex_lock(&tsd->mutex);
    
    if (tsd->data) {
        cJSON_Delete(tsd->data);
    }
    tsd->data = value;
    
    save_to_file_unlocked(tsd);
    pthread_mutex_unlock(&tsd->mutex);
}