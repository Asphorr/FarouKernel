#ifndef HEAP_H
#define HEAP_H

#include <stdbool.h>
#include <stdint.h>

// Heap structure
typedef struct heap_s {
    uint32_t capacity;      // Capacity of the heap
    uint32_t size;         // Current number of elements in the heap
    void** data;          // Array of pointers to heap elements
} heap_t;

// Function prototypes
heap_t* heap_create(uint32_t capacity);
void heap_destroy(heap_t* heap);
void heap_insert(heap_t* heap, void* element);
void heap_remove(heap_t* heap);
void heap_sort(heap_t* heap);

// Helper functions
static inline bool heap_is_empty(heap_t* heap);
static inline bool heap_is_full(heap_t* heap);
static inline void heap_swap(heap_t* heap, uint32_t i, uint32_t j);

#endif  // HEAP_H
