#include "thread_safe_data.h"
#include "debug_macros.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <libgen.h> // For dirname

static const char* DEFAULT_FILENAME = "users.json"; // Changed from "src/users.json"

// Forward declarations
static void load_from_file(ThreadSafeData* tsd);
static bool save_to_file_unlocked(ThreadSafeData* tsd);
static bool ensure_directory_exists(const char* filepath);

void tsd_init(ThreadSafeData* tsd) {
    tsd->filename = strdup(DEFAULT_FILENAME);
    if (pthread_mutex_init(&tsd->mutex, NULL) != 0) {
        DEBUG_PRINT("Mutex init failed\n");
        exit(EXIT_FAILURE);
    }
    tsd->data = NULL;
    load_from_file(tsd);
}

void tsd_destroy(ThreadSafeData* tsd) {
    pthread_mutex_lock(&tsd->mutex);
    if (tsd->data) {
        cJSON_Delete(tsd->data);
        tsd->data = NULL;
    }
    free(tsd->filename);
    pthread_mutex_unlock(&tsd->mutex);
    pthread_mutex_destroy(&tsd->mutex);
}

// Ensure directory exists
static bool ensure_directory_exists(const char* filepath) {
    char* path_copy = strdup(filepath);
    if (!path_copy) return false;
    
    char* dir = dirname(path_copy);
    
    // Check if the directory path is just "." (current directory)
    if (strcmp(dir, ".") == 0) {
        free(path_copy);
        return true; // Current directory always exists
    }
    
    // Try to create directory tree
    char command[512];
    snprintf(command, sizeof(command), "mkdir -p %s", dir);
    int result = system(command);
    
    free(path_copy);
    return (result == 0);
}

static void load_from_file(ThreadSafeData* tsd) {
    pthread_mutex_lock(&tsd->mutex);
    
    DEBUG_PRINT("Loading data from %s\n", tsd->filename);
    
    FILE* file = fopen(tsd->filename, "r");
    if (!file) {
        DEBUG_PRINT("File not found, initializing empty data\n");
        tsd->data = cJSON_CreateObject();
        if (tsd->data) {
            cJSON_AddItemToObject(tsd->data, "users", cJSON_CreateArray());
            save_to_file_unlocked(tsd);
        }
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

    char* buffer = malloc(length + 1);
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
    
    pthread_mutex_unlock(&tsd->mutex);
}

static bool save_to_file_unlocked(ThreadSafeData* tsd) {
    if (!tsd->data) {
        DEBUG_PRINT("No data to save\n");
        return false;
    }
    
    DEBUG_PRINT("Saving data to %s\n", tsd->filename);
    
    // Ensure directory exists
    if (!ensure_directory_exists(tsd->filename)) {
        DEBUG_PRINT("Failed to create directory for %s\n", tsd->filename);
        return false;
    }
    
    // Create temp file in same directory
    char temp_filename[256];
    snprintf(temp_filename, sizeof(temp_filename), "%s.tmp", tsd->filename);
    
    FILE* file = fopen(temp_filename, "w");
    if (!file) {
        DEBUG_PRINT("Failed to open temp file: %s\n", strerror(errno));
        return false;
    }

    char* json_str = cJSON_Print(tsd->data);
    if (!json_str) {
        DEBUG_PRINT("JSON print failed\n");
        fclose(file);
        remove(temp_filename);
        return false;
    }

    size_t len = strlen(json_str);
    size_t written = fwrite(json_str, 1, len, file);
    free(json_str);
    
    if (written != len) {
        DEBUG_PRINT("Write incomplete: %zu/%zu\n", written, len);
        fclose(file);
        remove(temp_filename);
        return false;
    }

    if (fflush(file) != 0 || fsync(fileno(file)) != 0) {
        DEBUG_PRINT("Flush/sync failed: %s\n", strerror(errno));
        fclose(file);
        remove(temp_filename);
        return false;
    }

    fclose(file);
    
    if (rename(temp_filename, tsd->filename) != 0) {
        DEBUG_PRINT("Rename failed: %s\n", strerror(errno));
        remove(temp_filename);
        return false;
    }

    return true;
}

cJSON* tsd_read(ThreadSafeData* tsd) {
    pthread_mutex_lock(&tsd->mutex);
    cJSON* copy = tsd->data ? cJSON_Duplicate(tsd->data, 1) : NULL;
    pthread_mutex_unlock(&tsd->mutex);
    return copy;
}

bool tsd_write(ThreadSafeData* tsd, cJSON* value) {
    bool result = false;
    pthread_mutex_lock(&tsd->mutex);
    
    if (tsd->data) {
        cJSON_Delete(tsd->data);
    }
    tsd->data = value;
    
    result = save_to_file_unlocked(tsd);
    pthread_mutex_unlock(&tsd->mutex);
    return result;
}