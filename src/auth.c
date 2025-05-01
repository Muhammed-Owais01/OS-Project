#include "auth.h"
#include "thread_safe_data.h"
#include <openssl/sha.h>
#include <string.h>
#include <stdlib.h>
#include <cstdio>

#define SALT_LENGTH 16
#define HASH_LENGTH (SHA256_DIGEST_LENGTH * 2 + 1)

static void hash_password(const char* password, char* output) {
    unsigned char salt[SALT_LENGTH];
    unsigned char hash[SHA256_DIGEST_LENGTH];
    char salted_password[SALT_LENGTH + strlen(password) + 1];
    
    // Generate random salt
    for (int i = 0; i < SALT_LENGTH; i++) {
        salt[i] = rand() % 256;
    }
    
    // Combine salt + password
    memcpy(salted_password, salt, SALT_LENGTH);
    strcpy(salted_password + SALT_LENGTH, password);
    
    // Hash with SHA-256
    SHA256((unsigned char*)salted_password, strlen(salted_password), hash);
    
    // Convert to hex string (salt + hash)
    for (int i = 0; i < SALT_LENGTH; i++) {
        sprintf(output + (i * 2), "%02x", salt[i]);
    }
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        sprintf(output + (SALT_LENGTH * 2) + (i * 2), "%02x", hash[i]);
    }
    output[HASH_LENGTH - 1] = '\0';
}

bool auth_signup(ThreadSafeData* tsd, const char* username, const char* password) {
    // Validate input
    if (!username || !password || strlen(username) == 0 || strlen(password) == 0) {
        return false;
    }

    char hashed_password[HASH_LENGTH];
    hash_password(password, hashed_password);
    
    pthread_mutex_lock(&tsd->mutex);
    
    // Get users array (your TSD already initializes this)
    cJSON* users = cJSON_GetObjectItem(tsd->data, "users");
    if (!users) {
        pthread_mutex_unlock(&tsd->mutex);
        return false;
    }
    
    // Check if username exists
    cJSON* user;
    cJSON_ArrayForEach(user, users) {
        cJSON* existing = cJSON_GetObjectItem(user, "username");
        if (existing && strcmp(existing->valuestring, username) == 0) {
            pthread_mutex_unlock(&tsd->mutex);
            return false; // Username taken
        }
    }
    
    // Create new user object
    cJSON* new_user = cJSON_CreateObject();
    cJSON_AddStringToObject(new_user, "username", username);
    cJSON_AddStringToObject(new_user, "password_hash", hashed_password);
    
    // Add to users array
    cJSON_AddItemToArray(users, new_user);
    
    // Save (using your existing TSD mechanism)
    tsd_write(tsd, tsd->data); 
    pthread_mutex_unlock(&tsd->mutex);
    return true;
}

bool auth_login(ThreadSafeData* tsd, const char* username, const char* password) {
    if (!username || !password) return false;
    
    char test_hash[HASH_LENGTH];
    hash_password(password, test_hash);
    
    pthread_mutex_lock(&tsd->mutex);
    bool authenticated = false;
    
    cJSON* users = cJSON_GetObjectItem(tsd->data, "users");
    if (users) {
        cJSON* user;
        cJSON_ArrayForEach(user, users) {
            cJSON* json_username = cJSON_GetObjectItem(user, "username");
            cJSON* stored_hash = cJSON_GetObjectItem(user, "password_hash");
            
            if (json_username && stored_hash && 
                strcmp(json_username->valuestring, username) == 0 &&
                strcmp(stored_hash->valuestring, test_hash) == 0) {
                authenticated = true;
                break;
            }
        }
    }
    
    pthread_mutex_unlock(&tsd->mutex);
    return authenticated;
}