#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>
#include <stdbool.h>

#define PAGE_SIZE 4096

typedef uint64_t page_table_entry;

void paging_init(void);
void *paging_map_page(uint64_t phys_addr, uint64_t virt_addr, uint64_t flags);
void paging_unmap_page(uint64_t virt_addr);
bool paging_is_page_present(uint64_t virt_addr);
uint64_t paging_get_physical_address(uint64_t virt_addr);

#endif
