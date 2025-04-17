// paging.c
#include "paging.h"
#include "pmm.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Макросы для работы с адресами
#define PAGE_MASK       0xFFFFFFFFFFFFF000
#define PAGE_FLAGS_MASK 0x0000000000000FFF
#define ALIGN_PAGE(addr) ((addr) & PAGE_MASK)

// Макросы для индексов таблиц
#define PML4_INDEX(addr) (((addr) >> 39) & 0x1FF)
#define PDPT_INDEX(addr) (((addr) >> 30) & 0x1FF)
#define PD_INDEX(addr)   (((addr) >> 21) & 0x1FF)
#define PT_INDEX(addr)   (((addr) >> 12) & 0x1FF)

// Флаги страниц
enum {
    PAGE_PRESENT  = 1 << 0,
    PAGE_WRITABLE = 1 << 1,
    PAGE_USER     = 1 << 2,
    PAGE_HUGE     = 1 << 7
};

static page_table_entry* pml4 = NULL;

static page_table_entry* get_next_level(page_table_entry* table, uint64_t index, bool create, uint64_t flags) {
    if (!(table[index] & PAGE_PRESENT)) {
        if (!create) return NULL;
        
        uint64_t new_table = (uint64_t)pmm_alloc_block();
        if (!new_table) return NULL;
        
        // Очищаем новую таблицу
        page_table_entry* new_table_ptr = (page_table_entry*)new_table;
        for (size_t i = 0; i < 512; i++) {
            new_table_ptr[i] = 0;
        }
        
        table[index] = ALIGN_PAGE(new_table) | (flags & PAGE_FLAGS_MASK) | PAGE_PRESENT;
    }
    return (page_table_entry*)(ALIGN_PAGE(table[index]) + KERNEL_VIRTUAL_BASE);
}

void paging_init(void) {
    // Выделяем выровненную память для PML4
    pml4 = (page_table_entry*)pmm_alloc_block_aligned(PAGE_SIZE);
    
    // Инициализируем PML4 нулями
    for (size_t i = 0; i < 512; i++) {
        pml4[i] = 0;
    }
    
    // Регистрируем PML4
    paging_load_directory((uint64_t)pml4 - KERNEL_VIRTUAL_BASE);
}

void* paging_map_page(uint64_t phys_addr, uint64_t virt_addr, uint64_t flags) {
    if (phys_addr & ~PAGE_MASK) return NULL;  // Проверка выравнивания
    
    page_table_entry* pdpt = get_next_level(pml4, PML4_INDEX(virt_addr), true, flags);
    if (!pdpt) return NULL;

    page_table_entry* pd = get_next_level(pdpt, PDPT_INDEX(virt_addr), true, flags);
    if (!pd) return NULL;

    page_table_entry* pt = get_next_level(pd, PD_INDEX(virt_addr), true, flags);
    if (!pt) return NULL;

    // Устанавливаем запись в таблицу страниц
    pt[PT_INDEX(virt_addr)] = ALIGN_PAGE(phys_addr) | (flags & PAGE_FLAGS_MASK) | PAGE_PRESENT;
    
    // Инвалидация TLB
    __asm__ volatile("invlpg (%0)" : : "r" (virt_addr) : "memory");
    
    return (void*)virt_addr;
}

void paging_unmap_page(uint64_t virt_addr) {
    page_table_entry* pdpt = get_next_level(pml4, PML4_INDEX(virt_addr), false, 0);
    if (!pdpt) return;

    page_table_entry* pd = get_next_level(pdpt, PDPT_INDEX(virt_addr), false, 0);
    if (!pd) return;

    page_table_entry* pt = get_next_level(pd, PD_INDEX(virt_addr), false, 0);
    if (!pt) return;

    // Очищаем запись и инвалидируем TLB
    pt[PT_INDEX(virt_addr)] = 0;
    __asm__ volatile("invlpg (%0)" : : "r" (virt_addr) : "memory");
}

bool paging_is_page_present(uint64_t virt_addr) {
    page_table_entry* pdpt = get_next_level(pml4, PML4_INDEX(virt_addr), false, 0);
    if (!pdpt) return false;

    page_table_entry* pd = get_next_level(pdpt, PDPT_INDEX(virt_addr), false, 0);
    if (!pd) return false;

    page_table_entry* pt = get_next_level(pd, PD_INDEX(virt_addr), false, 0);
    if (!pt) return false;

    return (pt[PT_INDEX(virt_addr)] & PAGE_PRESENT) != 0;
}

uint64_t paging_get_physical_address(uint64_t virt_addr) {
    page_table_entry* pdpt = get_next_level(pml4, PML4_INDEX(virt_addr), false, 0);
    if (!pdpt) return 0;

    page_table_entry* pd = get_next_level(pdpt, PDPT_INDEX(virt_addr), false, 0);
    if (!pd) return 0;

    page_table_entry* pt = get_next_level(pd, PD_INDEX(virt_addr), false, 0);
    if (!pt) return 0;

    return ALIGN_PAGE(pt[PT_INDEX(virt_addr)]) | (virt_addr & ~PAGE_MASK);
}

void paging_load_directory(uint64_t pml4_addr) {
    __asm__ volatile("mov %0, %%cr3" : : "r" (pml4_addr) : "memory");
}

uint64_t paging_get_directory(void) {
    uint64_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r" (cr3));
    return cr3;
}

// Дополнительные функции
void paging_map_range(uint64_t phys_start, uint64_t virt_start, size_t pages, uint64_t flags) {
    for (size_t i = 0; i < pages; i++) {
        paging_map_page(
            phys_start + i * PAGE_SIZE,
            virt_start + i * PAGE_SIZE,
            flags
        );
    }
}

void paging_identity_map(uint64_t start, size_t pages, uint64_t flags) {
    paging_map_range(start, start, pages, flags);
}
