// paging.c
#include "paging.h"
#include <stdalign.h>
#include <string.h>

alignas(4096) static uint64_t pml4[512];
alignas(4096) static uint64_t pdpt[512];
alignas(4096) static uint64_t pd[512];
alignas(4096) static uint64_t pt[512];

static struct {
    uint64_t* top_level;
    uint64_t kernel_start;
    uint64_t kernel_end;
} paging_state;

static inline void invlpg(uint64_t vaddr) {
    __asm__ __volatile__("invlpg (%0)" : : "r"(vaddr) : "memory");
}

static uint64_t* get_next_table(uint64_t* table, size_t index, bool create) {
    if (!(table[index] & PAGE_PRESENT)) {
        if (!create) return NULL;
        
        uint64_t* new_table = (uint64_t*)((uintptr_t)&pd + 4096 * index);
        memset(new_table, 0, 4096);
        table[index] = (uint64_t)new_table | PAGE_PRESENT | PAGE_RW;
    }
    return (uint64_t*)(table[index] & ~0xFFF);
}

void paging_init(void) {
    memset(pml4, 0, sizeof(pml4));
    memset(pdpt, 0, sizeof(pdpt));
    memset(pd, 0, sizeof(pd));
    memset(pt, 0, sizeof(pt));

    // Identity map first 4GB with 2MB pages
    for (uint64_t i = 0; i < 512; i++) {
        pd[i] = (i * PAGE_SIZE_2MB) | PAGE_PRESENT | PAGE_RW | PAGE_NX;
    }

    pdpt[0] = (uint64_t)pd | PAGE_PRESENT | PAGE_RW;
    pml4[0] = (uint64_t)pdpt | PAGE_PRESENT | PAGE_RW;

    // Self-map PML4 for later updates
    pml4[511] = (uint64_t)pml4 | PAGE_PRESENT | PAGE_RW;

    __asm__ __volatile__(
        "mov %0, %%cr3\n\t"
        "mov %%cr0, %%rax\n\t"
        "or $0x80000000, %%rax\n\t"
        "mov %%rax, %%cr0" 
        : : "r"(pml4) : "rax", "memory"
    );

    paging_state.top_level = pml4;
    enable_nx_bit();
}

bool map_memory(uint64_t vaddr, uint64_t paddr, uint64_t size, PageFlags flags) {
    const uint64_t end = vaddr + size;
    while (vaddr < end) {
        const size_t pml4_idx = (vaddr >> 39) & 0x1FF;
        const size_t pdpt_idx = (vaddr >> 30) & 0x1FF;
        const size_t pd_idx = (vaddr >> 21) & 0x1FF;
        const size_t pt_idx = (vaddr >> 12) & 0x1FF;

        uint64_t* pdpt_table = get_next_table(pml4, pml4_idx, true);
        uint64_t* pd_table = get_next_table(pdpt_table, pdpt_idx, true);
        
        if (size >= PAGE_SIZE_2MB && (vaddr & (PAGE_SIZE_2MB - 1)) == 0) {
            pd_table[pd_idx] = paddr | flags | PAGE_NX | (1 << 7);
            invlpg(vaddr);
            vaddr += PAGE_SIZE_2MB;
            paddr += PAGE_SIZE_2MB;
            size -= PAGE_SIZE_2MB;
        } else {
            uint64_t* pt_table = get_next_table(pd_table, pd_idx, true);
            pt_table[pt_idx] = paddr | flags | PAGE_NX;
            invlpg(vaddr);
            vaddr += 4096;
            paddr += 4096;
            size -= 4096;
        }
    }
    return true;
}

void enable_nx_bit(void) {
    uint32_t efer;
    __asm__ __volatile__("rdmsr" : "=A"(efer) : "c"(0xC0000080));
    efer |= 0x800;
    __asm__ __volatile__("wrmsr" : : "A"(efer), "c"(0xC0000080));
}
