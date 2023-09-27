#include "heap.h"

// Function to create a new heap with the given capacity
heap_t* heap_create(uint32_t capacity) {
    heap_t* heap = malloc(sizeof(struct heap_s));
    heap->capacity = capacity;
    heap->size = 0;
    heap->data = malloc(capacity * sizeof(void*));
    return heap;
}

// Function to destroy a heap and free its memory
void heap_destroy(heap_t* heap) {
    free(heap->data);
    free(heap);
}

// Function to insert an element into the heap
void heap_insert(heap_t* heap, void* element) {
    uint32_t idx = heap->size++;
    while (idx > 0) {
        uint32_t parentIdx = (idx - 1) / 2;
        if (heap->data[parentIdx] <= element) break;
        heap->data[idx] = heap->data[parentIdx];
        idx = parentIdx;
    }
    heap->data[idx] = element;
}

// Function to remove the root element from the heap and return it
void* heap_remove(heap_t* heap) {
    void* root = heap->data[0];
    heap->data[0] = heap->data[heap->size - 1];
    heap->size--;
    heap->data[heap->size] = NULL;
    return root;
}

// Function to sort the elements in the heap using the heap sort algorithm
void heap_sort(heap_t* heap) {
    uint32_t i, j, n = heap->size;
    for (i = n / 2 - 1; i >= 0; i--) {
        sink(heap, i, n);
    }
    for (i = n - 1; i > 0; i--) {
        swap(heap, 0, i);
        sink(heap, 0, i);
    }
}

// Helper function to swap two elements in the heap
static inline void swap(heap_t* heap, uint32_t i, uint32_t j) {
    void* temp = heap->data[i];
    heap->data[i] = heap->data[j];
    heap->data[j] = temp;
}

// Helper function to "sink" an element down the heap
static inline void sink(heap_t* heap, uint32_t i, uint32_t n) {
    uint32_t maxIndex = i;
    for (uint32_t j = i + 1; j < n; j++) {
        if (heap->data[j] > heap->data[maxIndex]) {
            maxIndex = j;
        }
    }
    if (maxIndex != i) {
        swap(heap, i, maxIndex);
        sink(heap, maxIndex, n);
    }
}
