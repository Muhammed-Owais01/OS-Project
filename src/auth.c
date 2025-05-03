#include "auth.h"
#include "thread_safe_data.h"
#include <openssl/sha.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

#define SALT_LENGTH 16
#define HASH_LENGTH (SHA256_DIGEST_LENGTH * 2 + 1)
#define TOKEN_LENGTH 32

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

static char* generate_token() {
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    char* token = malloc(TOKEN_LENGTH + 1);
    if (!token) return NULL;

    srand(time(NULL));
    for (int i = 0; i < TOKEN_LENGTH; i++) {
        int key = rand() % (sizeof(charset) - 1);
        token[i] = charset[key];
    }
    token[TOKEN_LENGTH] = '\0';
    return token;
}

bool auth_signup(ThreadSafeData* tsd, const char* username, const char* password) {
    if (!username || !password || strlen(username) == 0 || strlen(password) == 0) {
        return false;
    }

    char hashed_password[HASH_LENGTH];
    hash_password(password, hashed_password);
    
    pthread_mutex_lock(&tsd->mutex);
    
    cJSON* users = cJSON_GetObjectItem(tsd->data, "users");
    if (!users) {
        users = cJSON_AddArrayToObject(tsd->data, "users");
    }
    
    cJSON* user;
    cJSON_ArrayForEach(user, users) {
        cJSON* existing = cJSON_GetObjectItem(user, "username");
        if (existing && strcmp(existing->valuestring, username) == 0) {
            pthread_mutex_unlock(&tsd->mutex);
            return false;
        }
    }
    
    cJSON* new_user = cJSON_CreateObject();
    cJSON_AddStringToObject(new_user, "username", username);
    cJSON_AddStringToObject(new_user, "password_hash", hashed_password);
    cJSON_AddItemToArray(users, new_user);
    
    tsd_write(tsd, tsd->data);
    pthread_mutex_unlock(&tsd->mutex);
    return true;
}

char* auth_login(ThreadSafeData* tsd, const char* username, const char* password) {
    if (!username || !password) return NULL;
    
    char test_hash[HASH_LENGTH];
    hash_password(password, test_hash);
    
    pthread_mutex_lock(&tsd->mutex);
    char* token = NULL;
    
    cJSON* users = cJSON_GetObjectItem(tsd->data, "users");
    if (users) {
        cJSON* user;
        cJSON_ArrayForEach(user, users) {
            cJSON* json_username = cJSON_GetObjectItem(user, "username");
            cJSON* stored_hash = cJSON_GetObjectItem(user, "password_hash");
            
            if (json_username && stored_hash && 
                strcmp(json_username->valuestring, username) == 0 &&
                strcmp(stored_hash->valuestring, test_hash) == 0) {
                token = generate_token();
                break;
            }
        }
    }
    
    pthread_mutex_unlock(&tsd->mutex);
    return token;
}

bool auth_verify_token(const char* token, ThreadSafeData* tsd) {
    if (!token || strlen(token) != TOKEN_LENGTH) {
        return false;
    }
    return true;
}