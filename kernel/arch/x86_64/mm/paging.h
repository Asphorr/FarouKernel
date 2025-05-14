// paging.h
#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define PAGE_SIZE 4096
#define KERNEL_VIRTUAL_BASE 0xFFFF800000000000

typedef uint64_t page_table_entry;

// Основные функции
void paging_init(void);
void* paging_map_page(uint64_t phys_addr, uint64_t virt_addr, uint64_t flags);
void paging_unmap_page(uint64_t virt_addr);
bool paging_is_page_present(uint64_t virt_addr);
uint64_t paging_get_physical_address(uint64_t virt_addr);
void paging_load_directory(uint64_t pml4_addr);
uint64_t paging_get_directory(void);

// Дополнительные функции
void paging_map_range(uint64_t phys_start, uint64_t virt_start, size_t pages, uint64_t flags);
void paging_identity_map(uint64_t start, size_t pages, uint64_t flags);

// Флаги страниц
enum {
    PAGE_PRESENT  = 1 << 0,
    PAGE_WRITABLE = 1 << 1,
    PAGE_USER     = 1 << 2,
    PAGE_NO_EXEC  = 1 << 63
};

#endif // PAGING_H
