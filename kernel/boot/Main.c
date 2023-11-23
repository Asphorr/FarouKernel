#include <stdint.h>

#define VIDEO_MEMORY 0xb8000
#define WHITE_ON_BLACK 0x0f

void kernel_main(void) {
    // Initialize memory manager
    init_memory_manager();

    // Register system calls
    register_system_calls();

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

void init_memory_manager() {
    // Initialize your memory manager here
}

void register_system_calls() {
    // Register your system calls here
}
