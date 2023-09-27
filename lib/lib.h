#ifndef LIB_H
#define LIB_H

// Functions for working with strings
void* string_alloc(size_t len);
char* string_copy(const char* src, size_t len);
void string_free(void* ptr);

// Functions for working with arrays
void* array_alloc(size_t len, size_t elemsize);
void array_free(void* ptr);

// Functions for working with linked lists
void* linked_list_alloc(size_t len);
void linked_list_free(void* ptr);

// Functions for working with hash tables
void* hash_table_alloc(size_t len);
void hash_table_free(void* ptr);

// Functions for working with binary trees
void* binary_tree_alloc(size_t len);
void binary_tree_free(void* ptr);

// Functions for working with graphs
void* graph_alloc(size_t len);
void graph_free(void* ptr);

// Functions for working with matrices
void* matrix_alloc(size_t rows, size_t cols);
void matrix_free(void* ptr);

#endif  // LIB_H
