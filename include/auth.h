#ifndef AUTH_H
#define AUTH_H

#include "thread_safe_data.h"
#include <stdbool.h>

bool auth_signup(ThreadSafeData* tsd, const char* username, const char* password);
bool auth_login(ThreadSafeData* tsd, const char* username, const char* password);

#endif