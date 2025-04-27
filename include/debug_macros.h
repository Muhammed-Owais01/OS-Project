// debug_macros.h
#ifndef DEBUG_MACROS_H
#define DEBUG_MACROS_H

#include <stdio.h>

#define DEBUG_PRINT(fmt, ...) \
    do { fprintf(stderr, "[DEBUG] %s:%d:%s(): " fmt, __FILE__, \
                __LINE__, __func__, ##__VA_ARGS__); } while (0)

#define DEBUG_ENTER() DEBUG_PRINT("Entering\n")
#define DEBUG_EXIT() DEBUG_PRINT("Exiting\n")
#define DEBUG_PTR(ptr) DEBUG_PRINT("Pointer %s at %p\n", #ptr, (void*)ptr)

#endif