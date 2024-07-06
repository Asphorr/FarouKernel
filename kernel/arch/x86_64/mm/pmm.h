#ifndef PMM_H
#define PMM_H

#include <stdint.h>
#include <stdbool.h>

#define BLOCK_SIZE 4096

void pmm_init(uint64_t mem_size, uint64_t *bitmap);
void *pmm_alloc_block(void);
void pmm_free_block(void *p);
uint64_t pmm_get_memory_size(void);
uint64_t pmm_get_block_count(void);
uint64_t pmm_get_free_block_count(void);
uint64_t pmm_get_used_block_count(void);

#endif
