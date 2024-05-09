// paging.c
#include <stdint.h>
#include <string.h>
#include "paging.h"

uint64_t pml4_table[512] __attribute__((aligned(0x1000)));
uint64_t pdpt_table[512] __attribute__((aligned(0x1000)));
uint64_t pd_table[512] __attribute__((aligned(0x1000)));

void setup_paging() {
    memset(pml4_table, 0, sizeof(pml4_table));
    memset(pdpt_table, 0, sizeof(pdpt_table));
    memset(pd_table, 0, sizeof(pd_table));

    // Map the first 2 MiB of physical memory
    pd_table[0] = 0x83;  // Present, Read/Write, 2MB Page

    pdpt_table[0] = ((uint64_t)&pd_table) | 0x03;  // Present, Read/Write
    pml4_table[0] = ((uint64_t)&pdpt_table) | 0x03;  // Present, Read/Write

    // Load the new page tables
    __asm__ __volatile__("mov %0, %%cr3" : : "r"(&pml4_table));
}
