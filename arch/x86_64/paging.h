// paging.h
#pragma once
#include <stdint.h>
#include <stdbool.h>

#define PAGE_SIZE_2MB (2UL * 1024 * 1024)
#define PAGE_SIZE_1GB (1UL * 1024 * 1024 * 1024)
#define PAGE_NX       (1UL << 63)
#define PAGE_PRESENT  (1UL << 0)
#define PAGE_RW       (1UL << 1)
#define PAGE_USER     (1UL << 2)
#define PAGE_PAT      (1UL << 7)

typedef enum {
    PAGE_MAP_KERNEL_RW = PAGE_PRESENT | PAGE_RW,
    PAGE_MAP_KERNEL_RX = PAGE_PRESENT,
    PAGE_MAP_USER_RW = PAGE_PRESENT | PAGE_RW | PAGE_USER,
    PAGE_MAP_USER_RX = PAGE_PRESENT | PAGE_USER
} PageFlags;

void paging_init(void);
bool map_memory(uint64_t vaddr, uint64_t paddr, uint64_t size, PageFlags flags);
void enable_nx_bit(void);
