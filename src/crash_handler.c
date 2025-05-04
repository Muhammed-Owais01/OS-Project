#include "crash_handler.h"
#include <execinfo.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define BACKTRACE_SIZE 50

static void crash_handler(int sig) {
    void *array[BACKTRACE_SIZE];
    size_t size = backtrace(array, BACKTRACE_SIZE);
    
    fprintf(stderr, "\n=== CRASH DETECTED ===\n");
    fprintf(stderr, "Error: Signal %d\n", sig);
    fprintf(stderr, "Stack trace:\n");
    backtrace_symbols_fd(array, size, STDERR_FILENO);
    exit(1);
}

void install_crash_handler() {
    signal(SIGSEGV, crash_handler);  // Segmentation fault
    signal(SIGABRT, crash_handler);  // Abort signal
    signal(SIGILL, crash_handler);   // Illegal instruction
    signal(SIGFPE, crash_handler);   // Floating point exception
}