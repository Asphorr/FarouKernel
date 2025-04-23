#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h> // Requires Limine boot protocol headers

// --- Configuration & Constants ---

#define PAGE_SIZE 0x1000      // 4 KiB
#define PAGE_SHIFT 12
#define PAGE_MASK (~(PAGE_SIZE - 1))

// Virtual Memory Layout (Typical Higher-Half Kernel)
#define KERNEL_VMA_OFFSET 0xFFFF800000000000 // Kernel virtual addresses start here
#define HHDM_VMA_OFFSET   0xFFFF800000000000 // Higher Half Direct Map offset (maps all physical memory)

// Kernel Heap Configuration
#define KERNEL_HEAP_START (KERNEL_VMA_OFFSET + 0x40000000) // Example: Start heap 1GiB into kernel space
#define KERNEL_HEAP_INITIAL_SIZE (PAGE_SIZE * 256) // Reserve initial virtual space (1MB)
#define KERNEL_HEAP_MAX_SIZE (PAGE_SIZE * 1024 * 16) // Limit heap growth (64MB)

// Page Table Entry Flags (x86_64)
#define PTE_PRESENT (1ull << 0)
#define PTE_WRITE (1ull << 1)
#define PTE_USER (1ull << 2)
#define PTE_PWT (1ull << 3) // Page Write-Through
#define PTE_PCD (1ull << 4) // Page Cache Disable
#define PTE_ACCESSED (1ull << 5)
#define PTE_DIRTY (1ull << 6)
#define PTE_PAT (1ull << 7)  // Page Attribute Table index
#define PTE_GLOBAL (1ull << 8) // Global page (ignored in PML4E/PDPTE/PDE for 4k pages)
#define PTE_NX (1ull << 63) // No Execute bit
#define PTE_ADDR_MASK 0x000FFFFFFFFFF000 // Mask for physical address (bits 12-51)
#define PTE_FLAGS_MASK (PAGE_MASK | PTE_NX) // Mask for flags we preserve/set

// --- Utility Functions ---

// Basic kernel output (stub - replace with actual serial/framebuffer output)
#include <stdarg.h>
static void kprintf(const char* fmt, ...) {
    // In a real kernel: format string and output to console/serial
    va_list args;
    va_start(args, fmt);
    // ... implementation ...
    va_end(args);
    (void)fmt; // Suppress unused warning for stub
}

// Kernel panic (stub - replace with halt, debug info)
static void kpanic(const char* msg) {
    kprintf("KERNEL PANIC: %s\n", msg);
    // Disable interrupts
    asm volatile("cli");
    // Halt forever
    while (1) {
        asm volatile("hlt");
    }
}

// Basic assertion
#define ASSERT(cond) do { if (!(cond)) { kpanic("Assertion failed: " #cond); } } while(0)

// Basic memory manipulation (stubs - replace with optimized kernel versions)
void* memset(void* dest, int value, size_t count) {
    unsigned char* p = dest;
    while (count-- > 0) {
        *p++ = (unsigned char)value;
    }
    return dest;
}

void* memcpy(void* dest, const void* src, size_t count) {
    unsigned char* d = dest;
    const unsigned char* s = src;
    while (count-- > 0) {
        *d++ = *s++;
    }
    return dest;
}

// Align value down/up
static inline uint64_t align_down(uint64_t val, uint64_t align) {
    return val & ~(align - 1);
}
static inline uint64_t align_up(uint64_t val, uint64_t align) {
    return align_down(val + align - 1, align);
}

// --- Synchronization (Basic Spinlock) ---

typedef struct {
    volatile uint64_t locked;
} spinlock_t;

#define SPINLOCK_INIT {0}

// Acquire lock (using atomic test-and-set)
static inline void spin_lock(spinlock_t* lock) {
    while (__atomic_test_and_set(&lock->locked, __ATOMIC_ACQUIRE)) {
        // Spin hint (optional, helps CPU)
        asm volatile("pause");
    }
}

// Release lock
static inline void spin_unlock(spinlock_t* lock) {
    __atomic_clear(&lock->locked, __ATOMIC_RELEASE);
}

// --- Physical Memory Manager (PMM) - Bitmap Allocator ---

static uint64_t* pmm_bitmap = NULL;
static uint64_t pmm_bitmap_size_bytes = 0;
static uint64_t pmm_total_frames = 0;
static uint64_t pmm_used_frames = 0;
static uint64_t pmm_highest_frame = 0; // Highest usable frame index
static spinlock_t pmm_lock = SPINLOCK_INIT;
static uint64_t pmm_last_allocated_index = 0; // Hint for next allocation

// Sets a bit in the bitmap atomically
static inline void pmm_bitmap_set(uint64_t frame_index) {
    uint64_t idx = frame_index / 64;
    uint64_t bit = frame_index % 64;
    __atomic_fetch_or(&pmm_bitmap[idx], (1ull << bit), __ATOMIC_ACQ_REL);
}

// Clears a bit in the bitmap atomically
static inline void pmm_bitmap_clear(uint64_t frame_index) {
    uint64_t idx = frame_index / 64;
    uint64_t bit = frame_index % 64;
    __atomic_fetch_and(&pmm_bitmap[idx], ~(1ull << bit), __ATOMIC_ACQ_REL);
}

// Tests if a bit is set in the bitmap
static inline bool pmm_bitmap_test(uint64_t frame_index) {
    uint64_t idx = frame_index / 64;
    uint64_t bit = frame_index % 64;
    // Use relaxed atomic load as we only need atomicity relative to writes
    return (__atomic_load_n(&pmm_bitmap[idx], __ATOMIC_RELAXED) & (1ull << bit)) != 0;
}

// Finds the first count contiguous free frames
static int64_t pmm_find_first_free_contiguous(uint64_t count) {
    uint64_t consecutive_free = 0;
    // Start search from the last allocated index + 1 for better cache locality
    // and distribution of allocations. Wrap around if needed.
    uint64_t start_index = pmm_last_allocated_index + 1;

    for (uint64_t current_index = 0; current_index <= pmm_highest_frame; ++current_index) {
        uint64_t search_index = (start_index + current_index) % (pmm_highest_frame + 1);

        if (!pmm_bitmap_test(search_index)) {
            consecutive_free++;
            if (consecutive_free == count) {
                // Found a block, return the starting index
                return search_index - count + 1;
            }
        } else {
            // Reset counter if a used frame is encountered
            consecutive_free = 0;
        }
    }
    return -1; // Not found
}


// Allocates a single physical frame
uint64_t pmm_alloc_frame() {
    spin_lock(&pmm_lock);

    int64_t frame_index = pmm_find_first_free_contiguous(1);

    if (frame_index == -1) {
        // Try searching from the beginning if the hint didn't work
        pmm_last_allocated_index = (uint64_t)-1; // Reset hint
        frame_index = pmm_find_first_free_contiguous(1);
        if (frame_index == -1) {
            spin_unlock(&pmm_lock);
            kpanic("PMM: Out of physical memory!");
            return 0; // Should not be reached
        }
    }

    pmm_bitmap_set(frame_index);
    pmm_used_frames++;
    pmm_last_allocated_index = frame_index; // Update hint

    spin_unlock(&pmm_lock);

    uint64_t phys_addr = (uint64_t)frame_index * PAGE_SIZE;
    // kprintf("PMM: Allocated frame 0x%lx\n", phys_addr);

    // Zero the allocated frame for security and predictability
    void* virt_addr = phys_to_virt(phys_addr);
    memset(virt_addr, 0, PAGE_SIZE);

    return phys_addr;
}

// Frees a single physical frame
void pmm_free_frame(uint64_t phys_addr) {
    ASSERT((phys_addr % PAGE_SIZE) == 0); // Must be page aligned
    uint64_t frame_index = phys_addr / PAGE_SIZE;
    ASSERT(frame_index <= pmm_highest_frame); // Must be within manageable range

    spin_lock(&pmm_lock);

    if (!pmm_bitmap_test(frame_index)) {
        spin_unlock(&pmm_lock);
        kpanic("PMM: Attempted to free already free frame!");
        return;
    }

    pmm_bitmap_clear(frame_index);
    pmm_used_frames--;

    spin_unlock(&pmm_lock);
    // kprintf("PMM: Freed frame 0x%lx\n", phys_addr);
}

// --- Virtual Memory Manager (VMM) ---

// Page Map Level 4 Entry (PML4E) / Page Directory Pointer Table Entry (PDPTE)
// Page Directory Entry (PDE) / Page Table Entry (PTE)
typedef uint64_t pt_entry_t;

// Structure representing an address space (holds PML4 physical address)
typedef struct {
    uint64_t pml4_phys;
} address_space_t;

static address_space_t kernel_address_space;

// Converts physical address to virtual address in the HHDM
static inline void* phys_to_virt(uint64_t phys_addr) {
    return (void*)(phys_addr + HHDM_VMA_OFFSET);
}

// Converts virtual address in the HHDM to physical address
static inline uint64_t virt_to_phys(void* virt_addr) {
    return (uint64_t)virt_addr - HHDM_VMA_OFFSET;
}

// Reads the current CR3 register (physical address of PML4)
static inline uint64_t read_cr3() {
    uint64_t cr3_val;
    asm volatile("mov %%cr3, %0" : "=r"(cr3_val));
    return cr3_val;
}

// Loads a new value into CR3 (switches page table)
static inline void load_cr3(uint64_t cr3_val) {
    asm volatile("mov %0, %%cr3" ::"r"(cr3_val) : "memory");
}

// Invalidates a single page TLB entry
static inline void invlpg(void* virt_addr) {
    asm volatile("invlpg (%0)" ::"r"(virt_addr) : "memory");
}

// Gets the next level page table address from an entry
static inline pt_entry_t* vmm_get_next_level(pt_entry_t* entry, bool allocate_if_needed) {
    if (!(*entry & PTE_PRESENT)) {
        if (!allocate_if_needed) {
            return NULL;
        }
        // Allocate a new frame for the next level table
        uint64_t frame = pmm_alloc_frame();
        if (frame == 0) {
            kprintf("VMM: Failed to allocate frame for page table!\n");
            return NULL; // Out of physical memory
        }
        // Set the entry: Present, Writable, User (maybe), Frame Addr
        // User flag allows user access *to the table itself* if needed,
        // but actual page access depends on PTE flags. Keep kernel-only for now.
        *entry = frame | PTE_PRESENT | PTE_WRITE; // NX bit is implicitly 0
        // kprintf("VMM: Allocated PT frame 0x%lx for entry %p\n", frame, entry);
        return (pt_entry_t*)phys_to_virt(frame); // Return virtual address
    }
    // Table exists, return its virtual address
    return (pt_entry_t*)phys_to_virt(*entry & PTE_ADDR_MASK);
}

// Maps a virtual page to a physical frame in the given address space
bool vmm_map_page(address_space_t* space, void* virt_addr_in, uint64_t phys_addr, uint64_t flags) {
    ASSERT((phys_addr % PAGE_SIZE) == 0);
    uint64_t virt_addr = (uint64_t)virt_addr_in;
    ASSERT((virt_addr % PAGE_SIZE) == 0);

    // Ensure canonical address if needed (optional check)
    // ASSERT(((virt_addr >> 47) == 0) || ((virt_addr >> 47) == 0x1FFFF));

    uint64_t pml4_index = (virt_addr >> 39) & 0x1FF;
    uint64_t pdpt_index = (virt_addr >> 30) & 0x1FF;
    uint64_t pd_index = (virt_addr >> 21) & 0x1FF;
    uint64_t pt_index = (virt_addr >> 12) & 0x1FF;

    // Get virtual address of PML4
    pt_entry_t* pml4 = (pt_entry_t*)phys_to_virt(space->pml4_phys);

    // Walk the tables, allocating if necessary
    pt_entry_t* pdpt = vmm_get_next_level(&pml4[pml4_index], true);
    if (!pdpt) return false;
    pt_entry_t* pd = vmm_get_next_level(&pdpt[pdpt_index], true);
    if (!pd) return false;
    pt_entry_t* pt = vmm_get_next_level(&pd[pd_index], true);
    if (!pt) return false;

    // Set the final Page Table Entry (PTE)
    pt_entry_t* pte = &pt[pt_index];
    if (*pte & PTE_PRESENT) {
        // Page already mapped - this might be an error or require unmapping first
        kprintf("VMM: Warning - Remapping page at 0x%lx\n", virt_addr);
        // Consider freeing the old frame if ownership is clear
        // pmm_free_frame(*pte & PTE_ADDR_MASK);
    }

    *pte = (phys_addr & PTE_ADDR_MASK) | (flags & PTE_FLAGS_MASK) | PTE_PRESENT;

    // Invalidate TLB for this address
    invlpg(virt_addr_in);

    // kprintf("VMM: Mapped virt 0x%lx to phys 0x%lx flags 0x%lx\n", virt_addr, phys_addr, flags);
    return true;
}

// Unmaps a virtual page
// Returns the physical frame that was mapped, or 0 if not mapped.
// Does NOT free the physical frame. Caller must decide.
// Does NOT free intermediate page tables even if they become empty.
uint64_t vmm_unmap_page(address_space_t* space, void* virt_addr_in) {
    uint64_t virt_addr = (uint64_t)virt_addr_in;
    ASSERT((virt_addr % PAGE_SIZE) == 0);

    uint64_t pml4_index = (virt_addr >> 39) & 0x1FF;
    uint64_t pdpt_index = (virt_addr >> 30) & 0x1FF;
    uint64_t pd_index = (virt_addr >> 21) & 0x1FF;
    uint64_t pt_index = (virt_addr >> 12) & 0x1FF;

    pt_entry_t* pml4 = (pt_entry_t*)phys_to_virt(space->pml4_phys);

    // Walk tables without allocating
    pt_entry_t* pdpt = vmm_get_next_level(&pml4[pml4_index], false);
    if (!pdpt) return 0;
    pt_entry_t* pd = vmm_get_next_level(&pdpt[pdpt_index], false);
    if (!pd) return 0;
    pt_entry_t* pt = vmm_get_next_level(&pd[pd_index], false);
    if (!pt) return 0;

    pt_entry_t* pte = &pt[pt_index];
    if (!(*pte & PTE_PRESENT)) {
        return 0; // Page wasn't mapped
    }

    uint64_t phys_frame = *pte & PTE_ADDR_MASK;
    *pte = 0; // Clear the entry

    invlpg(virt_addr_in); // Invalidate TLB

    // kprintf("VMM: Unmapped virt 0x%lx (was phys 0x%lx)\n", virt_addr, phys_frame);
    return phys_frame;
}

// Switches the current address space
void vmm_switch_address_space(address_space_t* space) {
    load_cr3(space->pml4_phys);
    // kprintf("VMM: Switched address space to PML4 at 0x%lx\n", space->pml4_phys);
}

// --- Kernel Heap (kmalloc/kfree) - Simple Block Allocator ---

typedef struct kheap_block_header {
    size_t size; // Size of the data area *following* this header
    struct kheap_block_header* next_free;
    uint64_t magic; // For detecting corruption
} kheap_block_header_t;

#define KHEAP_MAGIC 0xDEADBEEFCAFEBABE
#define KHEAP_HEADER_SIZE sizeof(kheap_block_header_t)
#define KHEAP_MIN_ALIGNMENT 16 // x86_64 requires 16-byte alignment for SSE

static kheap_block_header_t* kheap_free_list = NULL;
static uintptr_t kheap_current_break = KERNEL_HEAP_START;
static uintptr_t kheap_max_break = KERNEL_HEAP_START + KERNEL_HEAP_MAX_SIZE;
static spinlock_t kheap_lock = SPINLOCK_INIT;

// Adds a block to the free list (maintains sorted order by address)
static void kheap_add_to_free_list(kheap_block_header_t* block) {
    block->magic = KHEAP_MAGIC; // Set magic when freeing/adding
    block->next_free = NULL;

    if (kheap_free_list == NULL || block < kheap_free_list) {
        block->next_free = kheap_free_list;
        kheap_free_list = block;
    } else {
        kheap_block_header_t* current = kheap_free_list;
        while (current->next_free != NULL && current->next_free < block) {
            current = current->next_free;
        }
        block->next_free = current->next_free;
        current->next_free = block;
    }
}

// Expands the kernel heap by mapping more pages
static bool kheap_expand(size_t bytes_needed) {
    uintptr_t old_break = kheap_current_break;
    uintptr_t new_break = align_up(old_break + bytes_needed, PAGE_SIZE);
    if (new_break > kheap_max_break) {
        kprintf("KHeap: Expansion failed - exceeds max heap size (0x%lx > 0x%lx)\n", new_break, kheap_max_break);
        return false;
    }

    // kprintf("KHeap: Expanding from 0x%lx to 0x%lx\n", old_break, new_break);

    for (uintptr_t addr = old_break; addr < new_break; addr += PAGE_SIZE) {
        uint64_t phys_frame = pmm_alloc_frame();
        if (phys_frame == 0) {
            kprintf("KHeap: Expansion failed - PMM out of memory during expansion\n");
            // Rollback? Difficult. Panic or return failure.
            // For simplicity, we don't rollback mappings here.
            return false;
        }

        if (!vmm_map_page(&kernel_address_space, (void*)addr, phys_frame, PTE_WRITE | PTE_NX)) {
            kprintf("KHeap: Expansion failed - VMM mapping error for 0x%lx\n", addr);
            pmm_free_frame(phys_frame); // Free the frame we couldn't map
            // Rollback? Difficult.
            return false;
        }
    }
    kheap_current_break = new_break;
    return true;
}

// Coalesces adjacent free blocks
static void kheap_coalesce_free_list() {
    kheap_block_header_t* current = kheap_free_list;
    while (current && current->next_free) {
        ASSERT(current->magic == KHEAP_MAGIC && current->next_free->magic == KHEAP_MAGIC);
        // If current block + its total size = next free block
        if ((uintptr_t)current + KHEAP_HEADER_SIZE + current->size == (uintptr_t)current->next_free) {
            // Coalesce
            current->size += KHEAP_HEADER_SIZE + current->next_free->size;
            current->next_free = current->next_free->next_free; // Remove next block
            // Stay on current block to check if it can coalesce further
        } else {
            current = current->next_free; // Move to next
        }
    }
}


// Allocate memory from kernel heap
void* kmalloc(size_t size) {
    if (size == 0) {
        return NULL;
    }

    // Align size and add header size
    size = align_up(size, KHEAP_MIN_ALIGNMENT);
    size_t total_needed = size + KHEAP_HEADER_SIZE;
    // Ensure total_needed is also aligned for the header placement
    total_needed = align_up(total_needed, KHEAP_MIN_ALIGNMENT);

    spin_lock(&kheap_lock);

    // 1. Search free list (first fit)
    kheap_block_header_t* current = kheap_free_list;
    kheap_block_header_t* prev = NULL;
    while (current) {
        ASSERT(current->magic == KHEAP_MAGIC); // Check for corruption
        if (current->size >= size) { // Found a suitable block (size is data area size)
            // Remove from free list
            if (prev) {
                prev->next_free = current->next_free;
            } else {
                kheap_free_list = current->next_free;
            }

            // Check if we can split the block
            // Need space for the allocated block's data AND a new header + min data size
            if (current->size >= size + KHEAP_HEADER_SIZE + KHEAP_MIN_ALIGNMENT) {
                // Calculate address of the new free block's header
                kheap_block_header_t* split_free = (kheap_block_header_t*)((uintptr_t)current + KHEAP_HEADER_SIZE + size);
                split_free->size = current->size - (size + KHEAP_HEADER_SIZE);
                // Add the split remainder back to the free list
                kheap_add_to_free_list(split_free);
                // Adjust size of the allocated block
                current->size = size;
                 // kprintf("KHeap: Split block %p (size %zu) into %zu and %zu\n", current, current->size + split_free->size + KHEAP_HEADER_SIZE, size, split_free->size);
            }
            // Else: Use the whole block, size is already >= requested size

            spin_unlock(&kheap_lock);
            // kprintf("KHeap: Allocated %zu bytes (total block %zu) from free list at %p\n", size, current->size + KHEAP_HEADER_SIZE, current);
            return (void*)((uintptr_t)current + KHEAP_HEADER_SIZE); // Return pointer *after* header
        }
        prev = current;
        current = current->next_free;
    }

    // 2. No suitable block in free list, expand the heap
    uintptr_t expansion_start = kheap_current_break;
    size_t expansion_needed = align_up(total_needed, PAGE_SIZE); // Expand by at least what's needed

    if (!kheap_expand(expansion_needed)) {
        spin_unlock(&kheap_lock);
        kprintf("KHeap: Allocation failed - cannot expand heap\n");
        return NULL; // Out of memory
    }

    // Create a new block header at the beginning of the expanded space
    kheap_block_header_t* new_block = (kheap_block_header_t*)expansion_start;
    new_block->size = expansion_needed - KHEAP_HEADER_SIZE; // Size of data area in the new large block

    // Add this large new block to the free list so it can be potentially split
    kheap_add_to_free_list(new_block);

    // Retry allocation (recursive call, but safe due to expansion)
    // This simplifies logic as the new block is now on the free list
    spin_unlock(&kheap_lock);
    // kprintf("KHeap: Expanded heap, retrying allocation for %zu bytes\n", size);
    return kmalloc(size); // Tail recursion might be optimized
}

// Free memory allocated by kmalloc
void kfree(void* ptr) {
    if (!ptr) {
        return;
    }

    kheap_block_header_t* block = (kheap_block_header_t*)((uintptr_t)ptr - KHEAP_HEADER_SIZE);

    spin_lock(&kheap_lock);

    // Basic sanity check
    ASSERT(block->magic == KHEAP_MAGIC); // Check if it *was* allocated

    // Add block back to free list (will handle sorting)
    kheap_add_to_free_list(block);

    // Coalesce free blocks
    kheap_coalesce_free_list();

    spin_unlock(&kheap_lock);
    // kprintf("KHeap: Freed block at %p (ptr %p)\n", block, ptr);
}


// --- Overall Memory Initialization ---

// Limine requests (ask bootloader for memory map and kernel address info)
static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0
};

static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST,
    .revision = 0
};

static volatile struct limine_kernel_address_request kernel_addr_request = {
    .id = LIMINE_KERNEL_ADDRESS_REQUEST,
    .revision = 0
};


// Main memory initialization function
void memory_init() {
    // --- Ensure Limine provided necessary info ---
    if (memmap_request.response == NULL || hhdm_request.response == NULL || kernel_addr_request.response == NULL) {
        kpanic("Memory Init: Missing required Limine information!");
    }
    struct limine_memmap_entry** memmap = memmap_request.response->entries;
    uint64_t memmap_entry_count = memmap_request.response->entry_count;
    uint64_t hhdm_offset = hhdm_request.response->offset;
    uint64_t kernel_phys_base = kernel_addr_request.response->physical_base;
    uint64_t kernel_virt_base = kernel_addr_request.response->virtual_base;

    kprintf("Memory Init: HHDM Offset: 0x%lx, Kernel Phys: 0x%lx, Kernel Virt: 0x%lx\n",
            hhdm_offset, kernel_phys_base, kernel_virt_base);
    ASSERT(hhdm_offset == HHDM_VMA_OFFSET); // Verify our assumption matches Limine

    // --- Initialize Physical Memory Manager (PMM) ---
    // 1. Find highest physical address and total usable memory
    uint64_t highest_addr = 0;
    uint64_t total_usable_mem = 0;
    for (uint64_t i = 0; i < memmap_entry_count; i++) {
        if (memmap[i]->type == LIMINE_MEMMAP_USABLE) {
            uint64_t top = memmap[i]->base + memmap[i]->length;
            total_usable_mem += memmap[i]->length;
            if (top > highest_addr) {
                highest_addr = top;
            }
            kprintf("  MemMap Usable: base=0x%lx, len=0x%lx\n", memmap[i]->base, memmap[i]->length);
        }
    }
    pmm_total_frames = highest_addr / PAGE_SIZE;
    pmm_highest_frame = pmm_total_frames - 1;
    pmm_bitmap_size_bytes = align_up((pmm_total_frames + 7) / 8, sizeof(uint64_t)); // Size in bytes, aligned

    kprintf("PMM: Highest Addr: 0x%lx, Total Frames: %lu, Bitmap Size: %lu bytes\n",
            highest_addr, pmm_total_frames, pmm_bitmap_size_bytes);

    // 2. Find a place for the bitmap itself within usable physical memory
    uint64_t bitmap_phys_addr = 0;
    for (uint64_t i = 0; i < memmap_entry_count; i++) {
        if (memmap[i]->type == LIMINE_MEMMAP_USABLE && memmap[i]->length >= pmm_bitmap_size_bytes) {
            // Found a spot. Place it at the start of this usable region.
            // Ensure it doesn't overlap with very low memory (e.g., below 1MB if needed).
            bitmap_phys_addr = align_up(memmap[i]->base, PAGE_SIZE);
            if (bitmap_phys_addr < 0x100000) bitmap_phys_addr = 0x100000; // Avoid low mem conflicts

            // Check if it still fits after alignment
            if (bitmap_phys_addr + pmm_bitmap_size_bytes <= memmap[i]->base + memmap[i]->length) {
                 kprintf("PMM: Placing bitmap at phys 0x%lx\n", bitmap_phys_addr);
                 break;
            } else {
                 bitmap_phys_addr = 0; // Reset if alignment pushed it out
            }
        }
    }
    if (bitmap_phys_addr == 0) {
        kpanic("PMM: Could not find suitable space for the bitmap!");
    }

    // 3. Initialize the bitmap (mark all as used initially)
    pmm_bitmap = (uint64_t*)phys_to_virt(bitmap_phys_addr);
    memset(pmm_bitmap, 0xFF, pmm_bitmap_size_bytes);
    pmm_used_frames = pmm_total_frames; // Assume all used initially

    // 4. Mark usable regions as free in the bitmap
    for (uint64_t i = 0; i < memmap_entry_count; i++) {
        if (memmap[i]->type == LIMINE_MEMMAP_USABLE) {
            uint64_t start_frame = align_up(memmap[i]->base, PAGE_SIZE) / PAGE_SIZE;
            uint64_t end_frame = align_down(memmap[i]->base + memmap[i]->length, PAGE_SIZE) / PAGE_SIZE;
            for (uint64_t frame = start_frame; frame < end_frame; frame++) {
                if (pmm_bitmap_test(frame)) { // Check if not already marked free
                    pmm_bitmap_clear(frame);
                    pmm_used_frames--;
                }
            }
        }
    }

    // 5. Mark the bitmap area itself as used (critical!)
    uint64_t bitmap_start_frame = bitmap_phys_addr / PAGE_SIZE;
    uint64_t bitmap_end_frame = align_up(bitmap_phys_addr + pmm_bitmap_size_bytes, PAGE_SIZE) / PAGE_SIZE;
    for (uint64_t frame = bitmap_start_frame; frame < bitmap_end_frame; frame++) {
        if (!pmm_bitmap_test(frame)) { // If it was marked free, mark it used
            pmm_bitmap_set(frame);
            pmm_used_frames++;
        }
    }
    kprintf("PMM: Initialized. Used Frames: %lu, Free Frames: %lu\n",
            pmm_used_frames, pmm_total_frames - pmm_used_frames);


    // --- Initialize Virtual Memory Manager (VMM) ---
    // 1. Get current PML4 (likely set up by Limine)
    kernel_address_space.pml4_phys = read_cr3();
    kprintf("VMM: Initial kernel PML4 (from CR3) at phys 0x%lx\n", kernel_address_space.pml4_phys);

    // 2. Ensure HHDM is mapped correctly by iterating through physical memory
    // Limine should have done this, but we can verify/extend if needed.
    // For every physical frame P, map P -> P + HHDM_VMA_OFFSET
    // This is crucial for phys_to_virt to work reliably.
    kprintf("VMM: Verifying/Extending HHDM...\n");
    for (uint64_t i = 0; i < memmap_entry_count; ++i) {
        uint64_t base = memmap[i]->base;
        uint64_t top = base + memmap[i]->length;
        // Map usable, bootloader reclaimable, kernel/modules, framebuffer regions
        if (memmap[i]->type == LIMINE_MEMMAP_USABLE ||
            memmap[i]->type == LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE ||
            memmap[i]->type == LIMINE_MEMMAP_KERNEL_AND_MODULES ||
            memmap[i]->type == LIMINE_MEMMAP_FRAMEBUFFER)
        {
            for (uint64_t paddr = align_down(base, PAGE_SIZE); paddr < top; paddr += PAGE_SIZE) {
                // Check if already mapped? For now, just map over potentially existing entry.
                vmm_map_page(&kernel_address_space, phys_to_virt(paddr), paddr, PTE_WRITE | PTE_NX); // Kernel R/W, No-Execute data
            }
        }
    }
    kprintf("VMM: HHDM mapping complete.\n");

    // --- Initialize Kernel Heap ---
    // The virtual address range KERNEL_HEAP_START -> KERNEL_HEAP_START + KERNEL_HEAP_MAX_SIZE
    // is reserved, but pages are allocated and mapped on demand by kmalloc -> kheap_expand.
    kheap_free_list = NULL;
    kheap_current_break = KERNEL_HEAP_START; // Set initial break
    kprintf("KHeap: Initialized. Ready at virt 0x%lx\n", KERNEL_HEAP_START);

    kprintf("Memory Management Initialized Successfully.\n");
}
