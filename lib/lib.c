#include <stdlib.h>
#include <stdio.h>
#include "lib.h"

// Functions for working with strings
void* string_alloc(size_t len) {
    void* ptr = malloc(len + 1);
    if (ptr == NULL) {
        fprintf(stderr, "Error: unable to allocate memory for string\n");
        exit(EXIT_FAILURE);
    }
    return ptr;
}

char* string_copy(const char* src, size_t len) {
    char* dst = string_alloc(len);
    memcpy(dst, src, len);
    dst[len] = '\0';
    return dst;
}

void string_free(void* ptr) {
    free(ptr);
}

// Functions for working with arrays
void* array_alloc(size_t len, size_t elemsize) {
    void* ptr = calloc(len, elemsize);
    if (ptr == NULL) {
        fprintf(stderr, "Error: unable to allocate memory for array\n");
        exit(EXIT_FAILURE);
    }
    return ptr;
}

void array_free(void* ptr) {
    free(ptr);
}

// Functions for working with linked lists
void* linked_list_alloc(size_t len) {
    void* ptr = malloc(sizeof(struct Node));
    if (ptr == NULL) {
        fprintf(stderr, "Error: unable to allocate memory for linked list\n");
        exit(EXIT_FAILURE);
    }
    ((struct Node*)ptr)->data = NULL;
    ((struct Node*)ptr)->next = NULL;
    return ptr;
}

void linked_list_free(void* ptr) {
    struct Node* curr = (struct Node*)ptr;
    while (curr != NULL) {
        struct Node* temp = curr->next;
        free(curr);
        curr = temp;
    }
}

// Functions for working with hash tables
void* hash_table_alloc(size_t len) {
    void* ptr = malloc(sizeof(struct HashTable));
    if (ptr == NULL) {
        fprintf(stderr, "Error: unable to allocate memory for hash table\n");
        exit(EXIT_FAILURE);
    }
    ((struct HashTable*)ptr)->size = len;
    ((struct HashTable*)ptr)->capacity = len;
    ((struct HashTable*)ptr)->keys = NULL;
    ((struct HashTable*)ptr)->values = NULL;
    return ptr;
}

void hash_table_free(void* ptr) {
    struct HashTable* tbl = (struct HashTable*)ptr;
    free(tbl->keys);
    free(tbl->values);
    free(ptr);
}

// Functions for working with binary trees
void* binary_tree_alloc(size_t len) {
    void* ptr = malloc(sizeof(struct Node));
    if (ptr == NULL) {
        fprintf(stderr, "Error: unable to allocate memory for binary tree\n");
        exit(EXIT_FAILURE);
    }
    ((struct Node*)ptr)->left = NULL;
    ((struct Node*)ptr)->right = NULL;
    ((struct Node*)ptr)->data = NULL;
    return ptr;
}

void binary_tree_free(void* ptr) {
    struct Node* curr = (struct Node*)ptr;
    while (curr != NULL) {
        struct Node* temp = curr->left;
        free(curr);
        curr = temp;
    }
}

// Functions for working with graphs
void* graph_alloc(size_t len) {
    void* ptr = malloc(sizeof(struct Node));
    if (ptr == NULL) {
        fprintf(stderr, "Error: unable to allocate memory for graph\n");
        exit(EXIT_FAILURE);
    }
    ((struct Node*)ptr)->neighbors = NULL;
    ((struct Node*)ptr)->data = NULL;
    return ptr;
}

void graph_free(void* ptr) {
    struct Node* curr = head;
while (curr != NULL) {
    struct Node* temp = curr->next;
    free(curr);
    curr = temp;
}
