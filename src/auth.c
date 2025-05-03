#include "auth.h"
#include "thread_safe_data.h"
#include "debug_macros.h"
#include <openssl/sha.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

#define SALT_LENGTH 16
#define HASH_LENGTH (SHA256_DIGEST_LENGTH * 2)
#define STORED_HASH_LENGTH (SALT_LENGTH * 2 + HASH_LENGTH + 1)
#define TOKEN_LENGTH 32

// Initialize random number generator once
__attribute__((constructor)) 
static void init_random() {
    srand(time(NULL));
}

static void hash_password(const char* password, const char* salt, char* output_hash) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    char salted_password[SALT_LENGTH + strlen(password) + 1];
    
    memcpy(salted_password, salt, SALT_LENGTH);
    strcpy(salted_password + SALT_LENGTH, password);
    
    SHA256((unsigned char*)salted_password, strlen(salted_password), hash);
    
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        sprintf(output_hash + (i * 2), "%02x", hash[i]);
    }
    output_hash[HASH_LENGTH] = '\0';
}

static char* generate_token() {
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    char* token = malloc(TOKEN_LENGTH + 1);
    if (!token) return NULL;

    for (int i = 0; i < TOKEN_LENGTH; i++) {
        token[i] = charset[rand() % (sizeof(charset) - 1)];
    }
    token[TOKEN_LENGTH] = '\0';
    return token;
}

bool auth_signup(ThreadSafeData* tsd, const char* username, const char* password) {
    DEBUG_PRINT("Attempting signup for: %s\n", username);
    
    if (!username || !password || strlen(username) == 0 || strlen(password) == 0) {
        DEBUG_PRINT("Invalid username or password\n");
        return false;
    }

    // Generate random salt
    unsigned char salt[SALT_LENGTH];
    for (int i = 0; i < SALT_LENGTH; i++) {
        salt[i] = rand() % 256;
    }

    // Convert salt to hex string
    char salt_hex[SALT_LENGTH * 2 + 1];
    for (int i = 0; i < SALT_LENGTH; i++) {
        sprintf(salt_hex + (i * 2), "%02x", salt[i]);
    }
    salt_hex[SALT_LENGTH * 2] = '\0';

    // Hash password with salt
    char hashed_password[HASH_LENGTH + 1];
    hash_password(password, (char*)salt, hashed_password);

    // Combine salt and hash for storage
    char stored_hash[STORED_HASH_LENGTH];
    snprintf(stored_hash, sizeof(stored_hash), "%s%s", salt_hex, hashed_password);

    pthread_mutex_lock(&tsd->mutex);
    DEBUG_PRINT("Mutex locked for signup\n");
    
    // Ensure data structure exists
    if (!tsd->data) {
        DEBUG_PRINT("Creating new data structure\n");
        tsd->data = cJSON_CreateObject();
        if (!tsd->data) {
            DEBUG_PRINT("Failed to create data object\n");
            goto cleanup;
        }
    }

    cJSON* users = cJSON_GetObjectItem(tsd->data, "users");
    if (!users) {
        DEBUG_PRINT("Creating users array\n");
        users = cJSON_AddArrayToObject(tsd->data, "users");
        if (!users) {
            DEBUG_PRINT("Failed to create users array\n");
            goto cleanup;
        }
    }
    
    // Check if username exists
    cJSON* user;
    cJSON_ArrayForEach(user, users) {
        cJSON* existing = cJSON_GetObjectItem(user, "username");
        if (existing && strcmp(existing->valuestring, username) == 0) {
            DEBUG_PRINT("Username %s already exists\n", username);
            goto cleanup;
        }
    }
    
    // Create new user
    cJSON* new_user = cJSON_CreateObject();
    if (!new_user) {
        DEBUG_PRINT("Failed to create user object\n");
        goto cleanup;
    }

    if (!cJSON_AddStringToObject(new_user, "username", username) ||
        !cJSON_AddStringToObject(new_user, "password_hash", stored_hash)) {
        DEBUG_PRINT("Failed to add user fields\n");
        cJSON_Delete(new_user);
        goto cleanup;
    }
    
    if (!cJSON_AddItemToArray(users, new_user)) {
        DEBUG_PRINT("Failed to add user to array\n");
        cJSON_Delete(new_user);
        goto cleanup;
    }
    
    // Save changes
    cJSON* data_to_write = cJSON_Duplicate(tsd->data, 1);
    if (!data_to_write) {
        DEBUG_PRINT("Failed to duplicate data for writing\n");
        goto cleanup;
    }

    // Write to file
    tsd_write(tsd, data_to_write);
    pthread_mutex_unlock(&tsd->mutex);
    
    DEBUG_PRINT("Signup successful for user %s\n", username);
    return true;

cleanup:
    pthread_mutex_unlock(&tsd->mutex);
    return false;
}

char* auth_login(ThreadSafeData* tsd, const char* username, const char* password) {
    DEBUG_PRINT("Attempting login for: %s\n", username);
    
    if (!username || !password || strlen(username) == 0 || strlen(password) == 0) {
        DEBUG_PRINT("Invalid username or password\n");
        return NULL;
    }

    pthread_mutex_lock(&tsd->mutex);
    char* token = NULL;
    
    if (!tsd->data) {
        DEBUG_PRINT("No data structure exists\n");
        goto cleanup;
    }

    cJSON* users = cJSON_GetObjectItem(tsd->data, "users");
    if (!users) {
        DEBUG_PRINT("No users array found\n");
        goto cleanup;
    }

    cJSON* user;
    cJSON_ArrayForEach(user, users) {
        cJSON* json_username = cJSON_GetObjectItem(user, "username");
        cJSON* stored_hash_json = cJSON_GetObjectItem(user, "password_hash");
        
        if (json_username && stored_hash_json && 
            strcmp(json_username->valuestring, username) == 0) {
            
            const char* stored_hash = stored_hash_json->valuestring;
            if (!stored_hash || strlen(stored_hash) != (SALT_LENGTH * 2 + HASH_LENGTH)) {
                DEBUG_PRINT("Invalid stored hash format\n");
                continue;
            }

            // Extract salt from stored hash
            char salt_hex[SALT_LENGTH * 2 + 1];
            strncpy(salt_hex, stored_hash, SALT_LENGTH * 2);
            salt_hex[SALT_LENGTH * 2] = '\0';

            // Convert hex salt back to bytes
            unsigned char salt[SALT_LENGTH];
            for (int i = 0; i < SALT_LENGTH; i++) {
                sscanf(salt_hex + (i * 2), "%02hhx", &salt[i]);
            }

            // Hash the provided password with the stored salt
            char test_hash[HASH_LENGTH + 1];
            hash_password(password, (char*)salt, test_hash);

            // Compare with stored hash
            if (strcmp(stored_hash + (SALT_LENGTH * 2), test_hash) == 0) {
                token = generate_token();
                DEBUG_PRINT("Login successful for user %s\n", username);
                break;
            } else {
                DEBUG_PRINT("Password verification failed for user %s\n", username);
            }
        }
    }
    
cleanup:
    pthread_mutex_unlock(&tsd->mutex);
    return token;
}

bool auth_verify_token(const char* token, ThreadSafeData* tsd) {
    (void)tsd; // Mark as unused to suppress warning
    
    if (!token || strlen(token) != TOKEN_LENGTH) {
        DEBUG_PRINT("Invalid token format\n");
        return false;
    }
    return true;
}