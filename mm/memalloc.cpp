#define MEMORY_SIZE (1 << 20) // 1MB of memory
#define BLOCK_SIZE 16 // minimum block size

struct memory_block {
    size_t size;
    bool is_free;
    struct memory_block* next;
    struct memory_block* buddy;
};

struct memory_block memory_blocks[MEMORY_SIZE / BLOCK_SIZE];

void init_memory() {
    // Initialize all memory blocks as free
    for (int i = 0; i < MEMORY_SIZE / BLOCK_SIZE; i++) {
        memory_blocks[i].size = BLOCK_SIZE;
        memory_blocks[i].is_free = true;
        memory_blocks[i].next = NULL;
        memory_blocks[i].buddy = NULL;
    }

    // Set up buddy pointers
    for (int i = 0; i < MEMORY_SIZE / BLOCK_SIZE; i++) {
        if (i % 2 == 0) {
            memory_blocks[i].buddy = &memory_blocks[i + 1];
            memory_blocks[i + 1].buddy = &memory_blocks[i];
        }
    }

    // Combine all blocks into one large free block
    struct memory_block* current = &memory_blocks[0];
    for (int i = 1; i < MEMORY_SIZE / BLOCK_SIZE; i++) {
        current->next = &memory_blocks[i];
        current = current->next;
    }
}

struct memory_block* find_free_block(size_t size) {
    struct memory_block* current = &memory_blocks[0];
    while (current != NULL) {
        if (current->is_free && current->size >= size) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

void split_block(struct memory_block* block, size_t size) {
    struct memory_block* new_block = (struct memory_block*)((uintptr_t)block + size);
    new_block->size = block->size - size;
    new_block->is_free = true;
    new_block->next = block->next;
    new_block->buddy = block->buddy;
    block->size = size;
    block->next = new_block;
    block->buddy = new_block->buddy;
    new_block->buddy->buddy = new_block;
}

void merge_blocks(struct memory_block* block) {
    if (block->buddy->is_free) {
        struct memory_block* buddy = block->buddy;
        block->size += buddy->size;
        block->next = buddy->next;
        block->buddy = buddy->buddy;
        block->buddy->buddy = block;
    }
}

void* kmalloc(size_t size) {
    // Round up to nearest power of 2
    size_t rounded_size = 1;
    while (rounded_size < size) {
        rounded_size <<= 1;
    }

    struct memory_block* block = find_free_block(rounded_size);
    if (block == NULL) {
        return NULL;
    }

    if (block->size > rounded_size) {
        split_block(block, rounded_size);
    }

    block->is_free = false;
    return block;
}

void kfree(void* ptr) {
    struct memory_block* block = (struct memory_block*)ptr;
    block->is_free = true;
    merge_blocks(block);
}
