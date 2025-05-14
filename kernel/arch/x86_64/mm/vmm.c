#include "vmm.h"
#include "pmm.h"
#include "paging.h"
#include <stdatomic.h>
#include <string.h>
#include <assert.h>

static vmm_address_space kernel_space = {
    .virtual_base = KERNEL_VIRTUAL_BASE,
    .virtual_top = KERNEL_VIRTUAL_BASE + 0x80000000, // 2GB
    .current_brk = KERNEL_VIRTUAL_BASE
};

static atomic_uintptr_t next_free_page = ATOMIC_VAR_INIT(KERNEL_VIRTUAL_BASE);
static atomic_flag vmm_lock = ATOMIC_FLAG_INIT;

static void vmm_spin_lock() {
    while (atomic_flag_test_and_set(&vmm_lock)) {
#ifdef __x86_64__
        __asm__ volatile("pause");
#endif
    }
}

static void vmm_spin_unlock() {
    atomic_flag_clear(&vmm_lock);
}

void vmm_init(void) {
    paging_init();
    
    // Identity map first 4MB
    vmm_map_range(0, 0, 1024, VMM_FLAG_PRESENT | VMM_FLAG_WRITABLE);
    
    // Map kernel higher half
    vmm_map_range(0, KERNEL_VIRTUAL_BASE, 512, VMM_FLAG_PRESENT | VMM_FLAG_WRITABLE);
    
    paging_load_directory(paging_get_directory());
}

static bool is_page_aligned(uint64_t addr) {
    return (addr & (PAGE_SIZE - 1)) == 0;
}

void* vmm_alloc_pages(size_t count) {
    if (count == 0) return NULL;

    vmm_spin_lock();
    
    uint64_t base_virt = atomic_load(&next_free_page);
    uint64_t current_virt = base_virt;
    
    for (size_t i = 0; i < count; i++) {
        void* phys = pmm_alloc_block();
        if (!phys) {
            // Rollback allocated pages
            while (i-- > 0) {
                pmm_free_block((void*)(current_virt - PAGE_SIZE * (i + 1)));
                paging_unmap_page(current_virt - PAGE_SIZE * (i + 1));
            }
            vmm_spin_unlock();
            return NULL;
        }
        
        if (!paging_map_page((uint64_t)phys, current_virt, 
                           VMM_FLAG_PRESENT | VMM_FLAG_WRITABLE)) {
            pmm_free_block(phys);
            vmm_spin_unlock();
            return NULL;
        }
        
        current_virt += PAGE_SIZE;
    }
    
    atomic_store(&next_free_page, current_virt);
    vmm_spin_unlock();
    
    return (void*)base_virt;
}

void vmm_free_pages(void* addr, size_t count) {
    if (!addr || count == 0 || !is_page_aligned((uint64_t)addr)) return;

    vmm_spin_lock();
    
    uint64_t virt = (uint64_t)addr;
    for (size_t i = 0; i < count; i++) {
        uint64_t phys = paging_get_physical_address(virt);
        if (phys) {
            pmm_free_block((void*)phys);
            paging_unmap_page(virt);
        }
        virt += PAGE_SIZE;
    }
    
    vmm_spin_unlock();
}

bool vmm_map_page(uint64_t phys, uint64_t virt, uint64_t flags) {
    if (!is_page_aligned(phys) || !is_page_aligned(virt)) return false;
    
    vmm_spin_lock();
    void* result = paging_map_page(phys, virt, flags);
    vmm_spin_unlock();
    
    return result != NULL;
}

void vmm_unmap_page(uint64_t virt) {
    if (!is_page_aligned(virt)) return;
    
    vmm_spin_lock();
    paging_unmap_page(virt);
    vmm_spin_unlock();
}

bool vmm_map_range(uint64_t phys_start, uint64_t virt_start, size_t pages, uint64_t flags) {
    if (!is_page_aligned(phys_start) || !is_page_aligned(virt_start)) return false;

    vmm_spin_lock();
    
    for (size_t i = 0; i < pages; i++) {
        uint64_t phys = phys_start + i * PAGE_SIZE;
        uint64_t virt = virt_start + i * PAGE_SIZE;
        
        if (!paging_map_page(phys, virt, flags)) {
            // Unmap already mapped pages
            while (i-- > 0) {
                paging_unmap_page(virt_start + i * PAGE_SIZE);
            }
            vmm_spin_unlock();
            return false;
        }
    }
    
    vmm_spin_unlock();
    return true;
}

void vmm_protect_page(uint64_t virt, uint64_t flags) {
    if (!is_page_aligned(virt)) return;
    
    vmm_spin_lock();
    
    page_table_entry* entry = (page_table_entry*)paging_get_physical_address(virt);
    if (entry) {
        *entry = (*entry & ~0xFFF) | (flags & 0xFFF);
        __asm__ volatile("invlpg (%0)" : : "r" (virt) : "memory");
    }
    
    vmm_spin_unlock();
}

// Дополнительные функции
uint64_t vmm_get_kernel_base(void) { return KERNEL_VIRTUAL_BASE; }

uint64_t vmm_get_free_virtual(void) {
    return atomic_load(&next_free_page);
}
