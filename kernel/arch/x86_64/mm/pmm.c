#include "pmm.h"
#include <string.h>
#include <stdatomic.h>
#include <assert.h>

static pmm_state_t state = {0};
static atomic_flag lock = ATOMIC_FLAG_INIT;

#define BITMAP_INDEX(block)  ((block) / BLOCKS_PER_LONG)
#define BITMAP_OFFSET(block) ((block) % BLOCKS_PER_LONG)

static void pmm_lock() {
    while (atomic_flag_test_and_set(&lock)) {
        // Спинлок с паузой для гипер-трединга
        #ifdef __x86_64__
        __asm__ volatile("pause");
        #endif
    }
}

static void pmm_unlock() {
    atomic_flag_clear(&lock);
}

void pmm_init(uint64_t mem_size, void* bitmap_base) {
    if (state.initialized) return;

    state.memory_size = mem_size;
    state.total_blocks = mem_size / BLOCK_SIZE;
    state.bitmap = (unsigned long*)bitmap_base;
    state.bitmap_size = (state.total_blocks + BLOCKS_PER_LONG - 1) / BLOCKS_PER_LONG;
    state.last_free_block = 0;
    
    // Пометить всю память как занятую
    memset(state.bitmap, 0xFF, state.bitmap_size * sizeof(unsigned long));
    state.used_blocks = state.total_blocks;
    
    state.initialized = true;
}

static void set_blocks(uint64_t block, size_t count, bool used) {
    for (size_t i = 0; i < count; i++) {
        size_t idx = BITMAP_INDEX(block + i);
        size_t off = BITMAP_OFFSET(block + i);
        
        if (used) {
            state.bitmap[idx] |= (1UL << off);
        } else {
            state.bitmap[idx] &= ~(1UL << off);
        }
    }
}

void* pmm_alloc_block(void) {
    return pmm_alloc_blocks(1);
}

void* pmm_alloc_blocks(size_t count) {
    if (!state.initialized || count == 0) return NULL;

    pmm_lock();
    
    uint64_t consecutive = 0;
    uint64_t start_block = state.last_free_block;
    
    for (uint64_t i = start_block; i < state.total_blocks; i++) {
        if (!(state.bitmap[BITMAP_INDEX(i)] & (1UL << BITMAP_OFFSET(i)))) {
            if (++consecutive == count) {
                uint64_t first_block = i - count + 1;
                set_blocks(first_block, count, true);
                state.used_blocks += count;
                state.last_free_block = i + 1;
                pmm_unlock();
                return (void*)(first_block * BLOCK_SIZE);
            }
        } else {
            consecutive = 0;
        }
    }
    
    pmm_unlock();
    return NULL;
}

void* pmm_alloc_block_aligned(uint64_t alignment) {
    assert((alignment % BLOCK_SIZE) == 0 && "Alignment must be multiple of BLOCK_SIZE");
    
    uint64_t align_blocks = alignment / BLOCK_SIZE;
    uint64_t attempts = 0;
    
    while (attempts++ < 3) {
        void* ptr = pmm_alloc_blocks(align_blocks);
        if (((uint64_t)ptr % alignment) == 0) {
            return ptr;
        }
        pmm_free_blocks(ptr, align_blocks);
    }
    return NULL;
}

void pmm_free_block(void* block) {
    pmm_free_blocks(block, 1);
}

void pmm_free_blocks(void* block, size_t count) {
    if (!block || !state.initialized || count == 0) return;

    uint64_t start = (uint64_t)block / BLOCK_SIZE;
    if (start + count > state.total_blocks) return;

    pmm_lock();
    
    set_blocks(start, count, false);
    state.used_blocks -= count;
    
    if (start < state.last_free_block) {
        state.last_free_block = start;
    }
    
    pmm_unlock();
}

void pmm_mark_region(uint64_t base, uint64_t size, bool used) {
    uint64_t align_base = (base + BLOCK_SIZE - 1) & ~(BLOCK_SIZE - 1);
    uint64_t align_size = size - (align_base - base);
    uint64_t blocks = align_size / BLOCK_SIZE;
    
    uint64_t start_block = align_base / BLOCK_SIZE;
    
    pmm_lock();
    set_blocks(start_block, blocks, used);
    state.used_blocks += used ? blocks : -blocks;
    pmm_unlock();
}

// Информационные функции
uint64_t pmm_get_total_memory(void) { return state.memory_size; }
uint64_t pmm_get_free_memory(void) { return (state.total_blocks - state.used_blocks) * BLOCK_SIZE; }
uint64_t pmm_get_used_memory(void) { return state.used_blocks * BLOCK_SIZE; }
bool pmm_is_initialized(void) { return state.initialized; }

void pmm_debug_dump(uint64_t entries) {
    printf("PMM State Dump:\n");
    printf("Total Memory: %llu MB\n", pmm_get_total_memory() / 1024 / 1024);
    printf("Used Memory:  %llu MB\n", pmm_get_used_memory() / 1024 / 1024);
    printf("Free Memory:  %llu MB\n", pmm_get_free_memory() / 1024 / 1024);
    
    printf("First %llu bitmap entries:\n", entries);
    for (uint64_t i = 0; i < entries; i++) {
        printf("Bitmap[%llu]: 0x%016lx\n", i, state.bitmap[i]);
    }
}
