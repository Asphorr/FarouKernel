#include "vmm.h"
#include "pmm.h"
#include "paging.h"

#define KERNEL_BASE 0xFFFF800000000000
#define PAGE_SIZE 4096

static uint64_t next_free_page = KERNEL_BASE;

void vmm_init(void) {
    paging_init();
    // Map the first 2MB of physical memory to the higher half
    for (uint64_t i = 0; i < 0x200000; i += PAGE_SIZE) {
        paging_map_page(i, KERNEL_BASE + i, PAGE_PRESENT | PAGE_WRITABLE);
    }
    paging_load_directory(paging_get_directory());
}

void *vmm_alloc_pages(uint64_t count) {
    void *ret = (void *)next_free_page;
    for (uint64_t i = 0; i < count; i++) {
        void *phys = pmm_alloc_block();
        if (!phys) {
            // Out of memory, free allocated pages and return NULL
            vmm_free_pages(ret, i);
            return NULL;
        }
        paging_map_page((uint64_t)phys, next_free_page, PAGE_PRESENT | PAGE_WRITABLE);
        next_free_page += PAGE_SIZE;
    }
    return ret;
}

void vmm_free_pages(void *addr, uint64_t count) {
    for (uint64_t i = 0; i < count; i++) {
        uint64_t virt = (uint64_t)addr + i * PAGE_SIZE;
        uint64_t phys = paging_get_physical_address(virt);
        if (phys) {
            pmm_free_block((void *)phys);
            paging_unmap_page(virt);
        }
    }
}

bool vmm_map_page(uint64_t phys, uint64_t virt, uint64_t flags) {
    return paging_map_page(phys, virt, flags) != NULL;
}

void vmm_unmap_page(uint64_t virt) {
    paging_unmap_page(virt);
}
