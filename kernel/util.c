#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"

void* malloc_or_die(size_t size) {
    void* ptr = malloc(size);
    if (ptr == NULL) {
        fprintf(stderr, "Failed to allocate %zu bytes\n", size);
        exit(EXIT_FAILURE);
    }
    return ptr;
}

void free_or_die(void* ptr) {
    if (ptr != NULL) {
        free(ptr);
    } else {
        fprintf(stderr, "Attempted to free null pointer\n");
        exit(EXIT_FAILURE);
    }
}

void print_error_and_exit(const char* message) {
    fprintf(stderr, "%s\n", message);
    exit(EXIT_FAILURE);
}
