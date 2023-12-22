#include <stdint.h>
#include <stddef.h>
#include <iostream>
#include <cstdlib>

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
  free(*ptr);
  *ptr = NULL;
}

int main() {
   // Declare a void pointer
   void* ptr;

   // Allocate memory for an integer
   ptr = malloc(sizeof(int));

   // Check if memory allocation was successful
   if (ptr == NULL) {
       std::cout << "Memory allocation failed!" << std::endl;
       return 1;
   }

   // Cast void pointer to int pointer and assign a value
   int* intPtr = static_cast<int*>(ptr);
   *intPtr = 10;

   // Print the value
   std::cout << "Value: " << *intPtr << std::endl;

   // Deallocate the memory
   deallocate_memory(&ptr);
    
}

void kernel_main(void) {
    
struct Block {
  size_t size;
  Block* next;
};

Block* head = nullptr;

bool init_memory_manager(size_t totalSize) {
  // Allocate memory for the head block
  head = static_cast<Block*>(malloc(totalSize));
  if (head == nullptr) {
      // Failed to allocate memory
      return false;
  }

  // Set the size of the head block to the total size
  head->size = totalSize - sizeof(Block);

  // There are no free blocks yet, so the next block is null
  head->next = nullptr;

  return true;
}

void* allocate_memory(size_t size) {
  Block* current = head;
  while (current != nullptr) {
      if (current->size >= size) {
          // Found a free block of sufficient size
          void* ptr = static_cast<void*>(current + 1);

          // Adjust the size of the remaining free space
          current->size -= size;

          // Move the head pointer forward
          head = current->next;

          return ptr;
      }

      // No suitable free block found, move to the next one
      current = current->next;
  }

  // No suitable free block found
  return nullptr;
}

void deallocate_memory(void* ptr) {
  // In a real implementation, you would need to merge adjacent free blocks
  // This is left as an exercise
}

void kernel_main(void) {
  if (!init_memory_manager(1024)) {
      std::cerr << "Error: Failed to initialize memory manager." << std::endl;
      return;
  }

  void* ptr = allocate_memory(sizeof(int));
  if (ptr == nullptr) {
      std::cerr << "Error: Failed to allocate memory." << std::endl;
      return;
  }

  int* intPtr = static_cast<int*>(ptr); {
  *intPtr = 10;

  std::cout << "Value: " << *intPtr << std::endl;

  deallocate_memory(ptr);
}



    
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
