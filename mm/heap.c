#include "heap.h"

typedef struct heap_node_s {
    void* data;
    int priority;
} heap_node_t;

typedef struct heap_s {
    size_t size;
    size_t capacity;
    heap_node_t** nodes;
} heap_t;

heap_t* heap_create(size_t capacity) {
    heap_t* heap = malloc(sizeof(heap_t));
    heap->nodes = calloc(capacity, sizeof(heap_node_t*));
    heap->size = 0;
    heap->capacity = capacity;
    return heap;
}

void heap_destroy(heap_t* heap) {
    free(heap->nodes);
    free(heap);
}

int heap_empty(const heap_t* heap) {
    return !heap || !heap->size;
}

void heap_push(heap_t* heap, void* data, int priority) {
    if (!heap || heap->size == heap->capacity) {
        return;
    }
    
    // Insert at end of array
    heap->nodes[heap->size++] = &((heap_node_t){data, priority});
    
    // Bubble up
    size_t index = heap->size - 1;
    while (index && heap->nodes[index]->priority > heap->nodes[(index - 1) / 2]->priority) {
        heap_swap(heap, index, (index - 1) / 2);
        index = (index - 1) / 2;
    }
}

void heap_pop(heap_t* heap) {
    if (!heap || heap->size == 0) {
        return;
    }
    
    // Remove last node
    heap->size--;
    heap->nodes[heap->size] = NULL;
    
    // Bubble down
    size_t index = 0;
    while ((index * 2) + 1 < heap->size) {
        size_t child = (index * 2) + 1;
        if (child + 1 < heap->size && heap->nodes[child + 1]->priority > heap->nodes[child]->priority) {
            child++;
        }
        
        if (heap->nodes[index]->priority > heap->nodes[child]->priority) {
            heap_swap(heap, index, child);
            index = child;
        } else {
            break;
        }
    }
}

void heap_swap(heap_t* heap, size_t index1, size_t index2) {
    heap_node_t* tmp = heap->nodes[index1];
    heap->nodes[index1] = heap->nodes[index2];
    heap->nodes[index2] = tmp;
}

void heap_sort(heap_t* heap) {
    size_t i, j, n = heap->size;
    for (i = n / 2 - 1; i >= 0; i--) {
        heap_sift_down(heap, i, n);
    }
    for (i = n - 1; i > 0; i--) {
        heap_swap(heap, 0, i);
        heap_sift_down(heap, 0, i);
    }
}

void heap_sift_down(heap_t* heap, size_t start, size_t end) {
    size_t root = start;
    while ((root * 2) + 1 <= end) {
        size_t child = (root * 2) + 1;
        if (child + 1 <= end && heap->nodes[child + 1]->priority > heap->nodes[child]->priority) {
            child++;
        }
        
        if (heap->nodes[root]->priority > heap->nodes[child]->priority) {
            heap_swap(heap, root, child);
            root = child;
        } else {
            break;
        }
    }
}
