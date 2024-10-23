#include "pmm.h"

static uint64_t memory_size = 0;
static uint64_t used_blocks = 0;
static uint64_t max_blocks = 0;
static uint64_t *memory_map = 0;

#define SET_BLOCK(bit) (memory_map[bit / 64] |= (1ULL << (bit % 64)))
#define UNSET_BLOCK(bit) (memory_map[bit / 64] &= ~(1ULL << (bit % 64)))
#define TEST_BLOCK(bit) ((memory_map[bit / 64] & (1ULL << (bit % 64))) != 0)

void pmm_init(uint64_t mem_size, uint64_t *bitmap) {
    memory_size = mem_size;
    memory_map = bitmap;
    max_blocks = mem_size / BLOCK_SIZE;
    used_blocks = max_blocks;

    // Initially, mark all memory as used
    for (uint64_t i = 0; i < max_blocks / 64; i++) {
        memory_map[i] = 0xFFFFFFFFFFFFFFFFULL;
    }
}

void *pmm_alloc_block(void) {
    if (used_blocks >= max_blocks) {
        return 0; // Out of memory
    }

    for (uint64_t i = 0; i < max_blocks; i++) {
        if (!TEST_BLOCK(i)) {
            SET_BLOCK(i);
            used_blocks++;
            return (void *)(i * BLOCK_SIZE);
        }
    }

    return 0; // No free blocks found
}

void pmm_free_block(void *p) {
    uint64_t addr = (uint64_t)p;
    uint64_t bit = addr / BLOCK_SIZE;

    if (bit >= max_blocks) {
        return; // Address out of range
    }

    if (TEST_BLOCK(bit)) {
        UNSET_BLOCK(bit);
        used_blocks--;
    }
}

uint64_t pmm_get_memory_size(void) {
    return memory_size;
}

uint64_t pmm_get_block_count(void) {
    return max_blocks;
}

uint64_t pmm_get_free_block_count(void) {
    return max_blocks - used_blocks;
}

uint64_t pmm_get_used_block_count(void) {
    return used_blocks;
}
