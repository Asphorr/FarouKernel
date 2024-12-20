#ifndef VMM_H
#define VMM_H

#include <stdint.h>
#include <stdbool.h>

void vmm_init(void);
void *vmm_alloc_pages(uint64_t count);
void vmm_free_pages(void *addr, uint64_t count);
bool vmm_map_page(uint64_t phys, uint64_t virt, uint64_t flags);
void vmm_unmap_page(uint64_t virt);

#endif
