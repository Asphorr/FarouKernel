#ifndef UTIL_H
#define UTIL_H

// Function declarations
void* malloc_or_die(size_t size);
void free_or_die(void* ptr);
void print_error_and_exit(const char* message);

// Additional utility functions
char* strdup_or_die(const char* string);
void* realloc_or_die(void* ptr, size_t size);
void memcpy_or_die(void* dest, void* src, size_t n);
void memmove_or_die(void* dest, void* src, size_t n);
void memset_or_die(void* ptr, int value, size_t n);
void bzero_or_die(void* ptr, size_t n);
void qsort_or_die(void* base, size_t num, size_t size, int (*compar)(const void*, const void*));

#endif /* UTIL_H */
