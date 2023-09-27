#include <kernel.h>
#include <stdint.h>
#include <string.h>

// Size of the memory pool in bytes
#define MEMORY_POOL_SIZE 0x100000

// Memory pool address
static void* memory_pool;

// Kernel virtual address space
static void* kernel_vas;

// User virtual address space
static void* user_vas;

// Maps a physical address to a virtual address
static inline void* phys_to_virt(uint32_t phys_addr) {
    return (void*)((uintptr_t)phys_addr - KERNEL_VIRTUAL_BASE);
}

// Maps a virtual address to a physical address
static inline uint32_t virt_to_phys(void* virt_addr) {
    return (uint32_t)((uintptr_t)virt_addr - KERNEL_VIRTUAL_BASE);
}

// Initializes the memory management system
void memory_init(void) {
    // Initialize the memory pool
    memory_pool = (void*)malloc(MEMORY_POOL_SIZE);
    memset(memory_pool, 0, MEMORY_POOL_SIZE);

    // Initialize the kernel virtual address space
    kernel_vas = (void*)malloc(KERNEL_VIRTUAL_SIZE);
    memset(kernel_vas, 0, KERNEL_VIRTUAL_SIZE);

    // Initialize the user virtual address space
    user_vas = (void*)malloc(USER_VIRTUAL_SIZE);
    memset(user_vas, 0, USER_VIRTUAL_SIZE);
}

// Allocates memory from the memory pool
void* memory_alloc(size_t size) {
    void* addr = memory_pool;

    // Check if the request fits within the memory pool
    if (size > MEMORY_POOL_SIZE - sizeof(void*)) {
        return NULL;
    }

    // Calculate the number of blocks required
    size_t num_blocks = (size + sizeof(void*) - 1) / sizeof(void*);

    // Allocate memory blocks
    for (size_t i = 0; i < num_blocks; i++) {
        // Check if the current block is available
        if (*(volatile uint8_t*)&memory_pool[i]) {
            // Mark the block as allocated
            *(volatile uint8_t*)&memory_pool[i] = 0;
            // Return the allocated memory block
            return &memory_pool[i];
        }
    }

    // If no blocks are available, allocate a new block
    void* new_block = malloc(size + sizeof(void*));
    if (!new_block) {
        return NULL;
    }

    // Mark the new block as allocated
    *(volatile uint8_t*)new_block = 0;

    // Chain the new block to the memory pool
    ((struct memory_block*)new_block)->prev = memory_pool;
    memory_pool = new_block;

    return new_block;
}

// Frees memory previously allocated with memory_alloc()
void memory_free(void* addr) {
    // Check if the address is valid
    if (!addr || !(*(volatile uint8_t*)addr)) {
        return;
    }

    // Get the previous block
    struct memory_block* prev = (struct memory_block*)((uintptr_t)addr - sizeof(void*));

    // Unlink the block from the memory pool
    prev->next = NULL;

    // Free the block
    free(addr);
}

// Maps a virtual address to a physical address
void* memory_map(void* virt_addr, size_t size) {
    // Check if the virtual address is valid
    if (!virt_addr || !size) {
        return NULL;
    }

    // Calculate the physical address
    uint32_t phys_addr = virt_to_phys(virt_addr);
#define MAP_SIZE 0x100000
#define MAX_NUM_BLOCKS 1024

// Structure to represent a memory block
typedef struct {
    void* addr;
    size_t size;
    struct memory_block* next;
} memory_block_t;

// Global variables
memory_block_t* memory_pool = NULL;
memory_block_t* kernel_vas = NULL;
memory_block_t* user_vas = NULL;

// Function to allocate memory from the memory pool
void* memory_alloc(size_t size) {
    void* addr = NULL;

    // Check if the requested size is valid
    if (size == 0) {
        return NULL;
    }

    // Search for a block that can accommodate the request
    memory_block_t* curr = memory_pool;
    while (curr != NULL && curr->size < size) {
        curr = curr->next;
    }

    // If a suitable block is found, split it into two smaller blocks
    if (curr != NULL) {
        // Calculate the size of the new block
        size_t new_size = curr->size - size;

        // Create a new block and set its attributes
        memory_block_t* new_block = (memory_block_t*)malloc(sizeof(struct memory_block));
        new_block->addr = curr->addr + size;
        new_block->size = new_size;
        new_block->next = curr->next;

        // Update the original block's size and next pointer
        curr->size = size;
        curr->next = new_block;

        // Return the newly created block
        return new_block->addr;
    } else {
        // No suitable block was found, so allocate a new one
        void* new_addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (new_addr == MAP_FAILED) {
            perror("mmap failed");
            exit(EXIT_FAILURE);
        }

        // Set up the new block's attributes
        memory_block_t* new_block = (memory_block_t*)malloc(sizeof(struct memory_block));
        new_block->addr = new_addr;
        new_block->size = size;
        new_block->next = memory_pool;

        // Add the new block to the memory pool
        memory_pool = new_block;

        return new_block->addr;
    }
}

// Function to free memory previously allocated with memory_alloc()
void memory_free(void* addr) {
    // Check if the address is valid
    if (!addr) {
        return;
    }

    // Find the block that contains the given address
    memory_block_t* curr = memory_pool;
    while (curr != NULL && curr->addr != addr) {
        curr = curr->next;
    }

    // If the block is not found, return error
    if (curr == NULL) {
        fprintf(stderr, "Memory block not found\n");
        exit(EXIT_FAILURE);
    }

    // Unlink the block from the memory pool
    curr->next = NULL;

    // Free the block
    munmap(curr->addr, curr->size);
    free(curr);
}

// Function to map a virtual address to a physical address
void* memory_map(void* virt_addr, size_t size) {
    // Check if the virtual address is valid
    if (!virt_addr || !size) {
        return NULL;
    }

    // Calculate the physical address
    uint32_t phys_addr = virt_to_phys(virt_addr);

    // Check if the physical address is valid
if (phys_addr >= KERNEL_VIRTUAL_BASE && phys_addr <= KERNEL_VIRTUAL_END) {
    // Map the physical address to a virtual address
    void* virt_addr = (void*)((uintptr_t)phys_addr - KERNEL_VIRTUAL_BASE + USER_VIRTUAL_BASE);

    // Check if the virtual address is already mapped
    if (munmap(virt_addr, size) == -1) {
        perror("munmap failed");
        exit(EXIT_FAILURE);
    }

    // Remap the virtual address to the physical address
    int ret = mremap(virt_addr, size, 0, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (ret == -1) {
        perror("mremap failed");
        exit(EXIT_FAILURE);
    }

    return virt_addr;
} else {
    // Invalid physical address
    return NULL;
}
