#ifndef UTIL_H
#define UTIL_H

// Function declarations
void* malloc_or_die(size_t size);
void free_or_die(void* ptr);
void print_error_and_exit(const char* message);

#endif /* UTIL_H */
