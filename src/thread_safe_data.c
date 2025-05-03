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

static const char* AUTH_FILENAME = "users.json";
static const char* DATA_FILENAME = "data.txt";

// Forward declarations
static void load_from_file(ThreadSafeData* tsd);
bool save_to_file_unlocked(ThreadSafeData* tsd);
static bool ensure_directory_exists(const char* filepath);

void tsd_init(ThreadSafeData* tsd) {
    tsd->auth_filename = strdup(AUTH_FILENAME);
    tsd->data_filename = strdup(DATA_FILENAME);
    if (pthread_mutex_init(&tsd->mutex, NULL) != 0) {
        DEBUG_PRINT("Mutex init failed\n");
        exit(EXIT_FAILURE);
    }
    tsd->auth_data = NULL;
    load_from_file(tsd);
}

void tsd_destroy(ThreadSafeData* tsd) {
    pthread_mutex_lock(&tsd->mutex);
    if (tsd->auth_data) {
        cJSON_Delete(tsd->auth_data);
        tsd->auth_data = NULL;
    }
    free(tsd->auth_filename);
    free(tsd->data_filename);
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
    
    // Load auth data
    DEBUG_PRINT("Loading auth data from %s\n", tsd->auth_filename);
    FILE* auth_file = fopen(tsd->auth_filename, "r");
    if (!auth_file) {
        DEBUG_PRINT("Auth file not found, initializing empty data\n");
        tsd->auth_data = cJSON_CreateObject();
        if (tsd->auth_data) {
            cJSON_AddItemToObject(tsd->auth_data, "users", cJSON_CreateArray());
            save_to_file_unlocked(tsd);
        }
    } else {
        fseek(auth_file, 0, SEEK_END);
        long length = ftell(auth_file);
        fseek(auth_file, 0, SEEK_SET);
        
        if (length > 0) {
            char* buffer = malloc(length + 1);
            if (buffer) {
                size_t read = fread(buffer, 1, length, auth_file);
                buffer[length] = '\0';
                tsd->auth_data = cJSON_Parse(buffer);
                free(buffer);
            }
        }
        fclose(auth_file);
    }
    
    pthread_mutex_unlock(&tsd->mutex);
}

bool save_to_file_unlocked(ThreadSafeData* tsd) {
    bool success = true;
    
    // Save auth data
    if (tsd->auth_data) {
        DEBUG_PRINT("Saving auth data to %s\n", tsd->auth_filename);
        if (!ensure_directory_exists(tsd->auth_filename)) {
            DEBUG_PRINT("Failed to create directory for %s\n", tsd->auth_filename);
            return false;
        }
        
        char temp_filename[256];
        snprintf(temp_filename, sizeof(temp_filename), "%s.tmp", tsd->auth_filename);
        
        FILE* file = fopen(temp_filename, "w");
        if (file) {
            char* json_str = cJSON_Print(tsd->auth_data);
            if (json_str) {
                fwrite(json_str, 1, strlen(json_str), file);
                free(json_str);
                fclose(file);
                rename(temp_filename, tsd->auth_filename);
            } else {
                fclose(file);
                remove(temp_filename);
                success = false;
            }
        } else {
            success = false;
        }
    }
    
    return success;
}

cJSON* tsd_read_auth(ThreadSafeData* tsd) {
    pthread_mutex_lock(&tsd->mutex);
    cJSON* copy = tsd->auth_data ? cJSON_Duplicate(tsd->auth_data, 1) : NULL;
    pthread_mutex_unlock(&tsd->mutex);
    return copy;
}

bool tsd_write_auth(ThreadSafeData* tsd, cJSON* value) {
    bool result = false;
    pthread_mutex_lock(&tsd->mutex);
    
    if (tsd->auth_data) {
        cJSON_Delete(tsd->auth_data);
    }
    tsd->auth_data = value;
    
    result = save_to_file_unlocked(tsd);
    pthread_mutex_unlock(&tsd->mutex);
    return result;
}

// New functions for handling text data
char* tsd_read_text(ThreadSafeData* tsd) {
    pthread_mutex_lock(&tsd->mutex);
    
    FILE* file = fopen(tsd->data_filename, "r");
    if (!file) {
        pthread_mutex_unlock(&tsd->mutex);
        return strdup("");  // Return empty string if file doesn't exist
    }
    
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char* buffer = malloc(length + 1);
    if (buffer) {
        size_t read = fread(buffer, 1, length, file);
        buffer[length] = '\0';
    }
    
    fclose(file);
    pthread_mutex_unlock(&tsd->mutex);
    return buffer ? buffer : strdup("");
}

bool tsd_write_text(ThreadSafeData* tsd, const char* text) {
    pthread_mutex_lock(&tsd->mutex);
    
    if (!ensure_directory_exists(tsd->data_filename)) {
        pthread_mutex_unlock(&tsd->mutex);
        return false;
    }
    
    FILE* file = fopen(tsd->data_filename, "a");  // Changed from "w" to "a" for append
    if (!file) {
        pthread_mutex_unlock(&tsd->mutex);
        return false;
    }
    
    bool success = (fwrite(text, 1, strlen(text), file) == strlen(text));
    if (success) {
        fwrite("\n", 1, 1, file);  // Add a newline after each entry
    }
    fclose(file);
    
    pthread_mutex_unlock(&tsd->mutex);
    return success;
}