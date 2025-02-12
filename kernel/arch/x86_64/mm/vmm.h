#ifndef VMM_H
#define VMM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define PAGE_SIZE 4096
#define KERNEL_VIRTUAL_BASE 0xFFFF800000000000

// Флаги виртуальной памяти
#define VMM_FLAG_PRESENT   (1 << 0)
#define VMM_FLAG_WRITABLE  (1 << 1)
#define VMM_FLAG_USER      (1 << 2)
#define VMM_FLAG_NOEXEC    (1 << 63)

typedef struct {
    uint64_t virtual_base;
    uint64_t virtual_top;
    uint64_t current_brk;
} vmm_address_space;

// Инициализация VMM
void vmm_init(void);

// Управление виртуальной памятью
void* vmm_alloc_pages(size_t count);
void vmm_free_pages(void* addr, size_t count);
bool vmm_map_page(uint64_t phys, uint64_t virt, uint64_t flags);
void vmm_unmap_page(uint64_t virt);

// Расширенные функции
bool vmm_map_range(uint64_t phys_start, uint64_t virt_start, size_t pages, uint64_t flags);
void vmm_protect_page(uint64_t virt, uint64_t flags);
void* vmm_create_address_space(void);
void vmm_switch_address_space(vmm_address_space* space);

// Информация
uint64_t vmm_get_kernel_base(void);
uint64_t vmm_get_free_virtual(void);

#endif // VMM_H
