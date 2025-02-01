#include "paging.h"
#include "pmm.h"

#define PML4_INDEX(addr) (((addr) >> 39) & 0x1FF)
#define PDPT_INDEX(addr) (((addr) >> 30) & 0x1FF)
#define PD_INDEX(addr) (((addr) >> 21) & 0x1FF)
#define PT_INDEX(addr) (((addr) >> 12) & 0x1FF)

#define PAGE_PRESENT (1ULL << 0)
#define PAGE_WRITABLE (1ULL << 1)
#define PAGE_USER (1ULL << 2)

static page_table_entry *pml4 = 0;

static page_table_entry *get_next_level(page_table_entry *table, uint64_t index, bool create) {
    if (!(table[index] & PAGE_PRESENT)) {
        if (!create) {
            return 0;
        }
        uint64_t new_table = (uint64_t)pmm_alloc_block();
        if (!new_table) {
            return 0;
        }
        table[index] = new_table | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;
    }
    return (page_table_entry *)((table[index] & ~0xFFF) + 0xFFFF800000000000);
}

void paging_init(void) {
    pml4 = pmm_alloc_block();
    for (int i = 0; i < 512; i++) {
        pml4[i] = 0;
    }
}

void *paging_map_page(uint64_t phys_addr, uint64_t virt_addr, uint64_t flags) {
    page_table_entry *pdpt, *pd, *pt;

    pdpt = get_next_level(pml4, PML4_INDEX(virt_addr), true);
    if (!pdpt) return 0;

    pd = get_next_level(pdpt, PDPT_INDEX(virt_addr), true);
    if (!pd) return 0;

    pt = get_next_level(pd, PD_INDEX(virt_addr), true);
    if (!pt) return 0;

    pt[PT_INDEX(virt_addr)] = (phys_addr & ~0xFFF) | flags | PAGE_PRESENT;

    return (void *)virt_addr;
}

void paging_unmap_page(uint64_t virt_addr) {
    page_table_entry *pdpt, *pd, *pt;

    pdpt = get_next_level(pml4, PML4_INDEX(virt_addr), false);
    if (!pdpt) return;

    pd = get_next_level(pdpt, PDPT_INDEX(virt_addr), false);
    if (!pd) return;

    pt = get_next_level(pd, PD_INDEX(virt_addr), false);
    if (!pt) return;

    pt[PT_INDEX(virt_addr)] = 0;
}

bool paging_is_page_present(uint64_t virt_addr) {
    page_table_entry *pdpt, *pd, *pt;

    pdpt = get_next_level(pml4, PML4_INDEX(virt_addr), false);
    if (!pdpt) return false;

    pd = get_next_level(pdpt, PDPT_INDEX(virt_addr), false);
    if (!pd) return false;

    pt = get_next_level(pd, PD_INDEX(virt_addr), false);
    if (!pt) return false;

    return (pt[PT_INDEX(virt_addr)] & PAGE_PRESENT) != 0;
}

uint64_t paging_get_physical_address(uint64_t virt_addr) {
    page_table_entry *pdpt, *pd, *pt;

    pdpt = get_next_level(pml4, PML4_INDEX(virt_addr), false);
    if (!pdpt) return 0;

    pd = get_next_level(pdpt, PDPT_INDEX(virt_addr), false);
    if (!pd) return 0;

    pt = get_next_level(pd, PD_INDEX(virt_addr), false);
    if (!pt) return 0;

    if (!(pt[PT_INDEX(virt_addr)] & PAGE_PRESENT)) {
        return 0;
    }

    return (pt[PT_INDEX(virt_addr)] & ~0xFFF) | (virt_addr & 0xFFF);
}

void paging_load_directory(uint64_t pml4_addr) {
    __asm__ volatile("mov %0, %%cr3" : : "r" (pml4_addr));
}

uint64_t paging_get_directory(void) {
    uint64_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r" (cr3));
    return cr3;
}
