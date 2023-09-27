#include "util.h"

void* malloc_or_die(size_t size) {
    void* ptr = malloc(size);
    if (ptr == NULL) {
        fprintf(stderr, "Failed to allocate %zu bytes\n", size);
        exit(1);
    }
    return ptr;
}

void free_or_die(void* ptr) {
    if (ptr == NULL) {
        fprintf(stderr, "Attempted to free null pointer\n");
        exit(1);
    }
    free(ptr);
}

void print_error_and_exit(const char* message) {
    fprintf(stderr, "%s\n", message);
    exit(1);
}
