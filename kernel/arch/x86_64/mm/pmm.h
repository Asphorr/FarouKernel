#ifndef PMM_H
#define PMM_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define BLOCK_SIZE      4096
#define BLOCKS_PER_BYTE 8
#define BLOCKS_PER_LONG (sizeof(unsigned long) * BLOCKS_PER_BYTE)

typedef struct {
    uint64_t total_blocks;
    uint64_t used_blocks;
    uint64_t memory_size;
    unsigned long* bitmap;
    uint64_t bitmap_size;
    uint64_t last_free_block;
    bool initialized;
} pmm_state_t;

// Инициализация PMM
void pmm_init(uint64_t mem_size, void* bitmap_base);

// Основные операции
void* pmm_alloc_block(void);
void* pmm_alloc_blocks(size_t count);
void* pmm_alloc_block_aligned(uint64_t alignment);
void pmm_free_block(void* block);
void pmm_free_blocks(void* block, size_t count);

// Информация о состоянии
uint64_t pmm_get_total_memory(void);
uint64_t pmm_get_free_memory(void);
uint64_t pmm_get_used_memory(void);
bool pmm_is_initialized(void);

// Вспомогательные функции
void pmm_mark_region(uint64_t base, uint64_t size, bool used);
void pmm_debug_dump(uint64_t entries);

#endif // PMM_H
