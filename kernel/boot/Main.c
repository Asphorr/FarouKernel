#include <stdint.h>
#include <stddef.h>

#define VIDEO_MEMORY 0xb8000
#define WHITE_ON_BLACK 0x0f

#define MEMORY_SIZE 1024 // 1KB of memory

uint8_t memory[MEMORY_SIZE];

void* allocate_memory(size_t size) {
    static size_t allocated = 0;

    if (allocated + size > MEMORY_SIZE) {
        return NULL; // out of memory
    }

    void* ptr = &memory[allocated];
    allocated += size;

    return ptr;
}

void deallocate_memory(void* ptr) {
    // In a real memory manager, you would need to implement this
    // For simplicity, we're not going to do anything here
}

void kernel_main(void) {
    // Initialize memory manager
    if (!init_memory_manager()) {
        // Handle error
    }

    // Register system calls
    if (!register_system_calls()) {
        // Handle error
    }

    // Print a string to the screen
    print_string("Hello, World!");
}

void print_string(const char* str) {
    char *vidmem = (char*)VIDEO_MEMORY;

    for (int i = 0; str[i] != '\0'; i++) {
        vidmem[i*2] = str[i]; // character
        vidmem[i*2+1] = WHITE_ON_BLACK; // attributes
    }
}

int init_memory_manager() {
    // Initialize your memory manager here
    // Return 0 on success, -1 on failure
}

int register_system_calls() {
    // Register your system calls here
    // Return 0 on success, -1 on failure
}
