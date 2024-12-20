#include <stdint.h>
#include <stddef.h>
#include <iostream>
#include <cstdlib>

const uintptr_t VIDEO_MEMORY = 0xb8000;
const uint8_t WHITE_ON_BLACK = 0x0f;
const size_t MEMORY_SIZE = 1024; // 1KB of memory

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
    
    try {
  if (!init_memory_manager()) {
      // Handle error
  }
} catch (const std::runtime_error& e) {
  // Log the error message
  std::cerr << "Error: " << e.what() << std::endl;
  // Handle the error appropriately
  // For example, you might want to terminate the program
  exit(EXIT_FAILURE);
    }
        
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
// Structure for our message queue
struct msgbuf {
  long mtype;
  char mtext[80];
};

int register_system_calls() {
  // Define the file path
  const char* file_path = "/path/to/your/file";

  // Open the file
  int fd = open(file_path, O_RDONLY);
  if (fd == -1) {
      perror("Failed to open the file");
      return -1;
  }

  // Read data from the file
  char buffer[100];
  ssize_t bytesRead = read(fd, buffer, sizeof(buffer) - 1);
  if (bytesRead == -1) {
      perror("Failed to read the file");
      close(fd);
      return -1;
  }

  // Null terminate the buffer
  buffer[bytesRead] = '\0';

  // Generate a unique key for our message queue
  key_t key = ftok("progfile", 65);
  int msgid = msgget(key, 0666 | IPC_CREAT);
  if (msgid == -1) {
      perror("Failed to create message queue");
      return -1;
  }

  // Send the file content as a message to the queue
  struct msgbuf message;
  message.mtype = 1;
  strncpy(message.mtext, buffer, sizeof(message.mtext));

  if (msgsnd(msgid, &message, sizeof(message), 0) == -1) {
      perror("Failed to send message");
      return -1;
  }

  // Receive a message from the queue
  if (msgrcv(msgid, &message, sizeof(message), 1, 0) == -1) {
      perror("Failed to receive message");
      return -1;
  }

  // Print the received message
  printf("Received message: %s\n", message.mtext);

  // Remove the message queue
  if (msgctl(msgid, IPC_RMID, NULL) == -1) {
      perror("Failed to remove message queue");
      return -1;
  }

  // Close the file
  if (close(fd) == -1) {
      perror("Failed to close the file");
      return -1;
  }

  return 0;
}
