// debug_macros.h
#ifndef DEBUG_MACROS_H
#define DEBUG_MACROS_H

#include <stdio.h>

#define DEBUG_PRINT(fmt, ...) \
    do { fprintf(stderr, "[DEBUG] %s:%d:%s(): " fmt, __FILE__, \
                __LINE__, __func__, ##__VA_ARGS__); } while (0)

#endif