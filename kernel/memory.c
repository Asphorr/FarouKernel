#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdarg.h> // For va_list, etc.
#include <limine.h> // Requires Limine boot protocol headers

// --- Configuration & Constants ---

#define PAGE_SIZE 0x1000      // 4 KiB
#define PAGE_SHIFT 12
#define PAGE_MASK (~(PAGE_SIZE - 1))

// Virtual Memory Layout (Typical Higher-Half Kernel)
#define KERNEL_VMA_OFFSET 0xFFFF800000000000 // Kernel virtual addresses start here
// HHDM_VMA_OFFSET will be obtained from Limine at runtime

// Kernel Heap Configuration
#define KERNEL_HEAP_START (KERNEL_VMA_OFFSET + 0x40000000) // Example: Start heap 1GiB into kernel space
#define KERNEL_HEAP_INITIAL_SIZE (PAGE_SIZE * 256) // Reserve initial virtual space (1MB) - Not directly used by this allocator
#define KERNEL_HEAP_MAX_SIZE (PAGE_SIZE * 1024 * 16) // Limit heap growth (64MB)

// Page Table Entry Flags (x86_64)
#define PTE_PRESENT (1ull << 0)
#define PTE_WRITE (1ull << 1)
#define PTE_USER (1ull << 2) // User accessible page
#define PTE_PWT (1ull << 3) // Page Write-Through
#define PTE_PCD (1ull << 4) // Page Cache Disable
#define PTE_ACCESSED (1ull << 5)
#define PTE_DIRTY (1ull << 6)
#define PTE_PAT (1ull << 7)  // Page Attribute Table index
#define PTE_GLOBAL (1ull << 8) // Global page (ignored in PML4E/PDPTE/PDE for 4k pages)
#define PTE_NX (1ull << 63) // No Execute bit
#define PTE_ADDR_MASK 0x000FFFFFFFFFF000 // Mask for physical address (bits 12-51)
#define PTE_FLAGS_MASK (~PTE_ADDR_MASK) // Mask for all flags

// --- Basic I/O (for kprintf) ---

#define SERIAL_COM1_PORT 0x3F8

static inline void outb(uint16_t port, uint8_t value) {
    asm volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// Basic Serial Port Initialization (minimal)
static void serial_init() {
    outb(SERIAL_COM1_PORT + 1, 0x00); // Disable interrupts
    outb(SERIAL_COM1_PORT + 3, 0x80); // Enable DLAB (set baud rate divisor)
    outb(SERIAL_COM1_PORT + 0, 0x01); // Set divisor to 1 (lo byte) 115200 baud
    outb(SERIAL_COM1_PORT + 1, 0x00); //                  (hi byte)
    outb(SERIAL_COM1_PORT + 3, 0x03); // 8 bits, no parity, one stop bit
    outb(SERIAL_COM1_PORT + 2, 0xC7); // Enable FIFO, clear them, with 14-byte threshold
    outb(SERIAL_COM1_PORT + 4, 0x0B); // IRQs enabled, RTS/DSR set
    // Test output
    outb(SERIAL_COM1_PORT, '!');
}

// Check if serial transmit buffer is empty
static int serial_is_transmit_empty() {
   return inb(SERIAL_COM1_PORT + 5) & 0x20;
}

// Write character to serial port
static void serial_putchar(char a) {
   while (serial_is_transmit_empty() == 0); // Wait until buffer is empty
   outb(SERIAL_COM1_PORT, a);
}

// Write string to serial port
static void serial_puts(const char* str) {
    for (size_t i = 0; str[i] != '\0'; i++) {
        serial_putchar(str[i]);
    }
}

// Simple integer to hex string conversion (recursive helper)
static void kprint_hex_recursive(uint64_t n) {
    const char* hex_chars = "0123456789abcdef";
    if (n == 0) return;
    kprint_hex_recursive(n / 16);
    serial_putchar(hex_chars[n % 16]);
}

// Print unsigned 64-bit integer in hexadecimal
static void kprint_hex(uint64_t n) {
    if (n == 0) {
        serial_putchar('0');
    } else {
        kprint_hex_recursive(n);
    }
}

// Kernel printf implementation (basic: %s, %c, %lx, %p)
static void kprintf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    for (const char* p = fmt; *p != '\0'; p++) {
        if (*p != '%') {
            serial_putchar(*p);
            continue;
        }

        p++; // Move past '%'
        switch (*p) {
            case 'c': {
                char c = (char)va_arg(args, int); // char is promoted to int
                serial_putchar(c);
                break;
            }
            case 's': {
                const char* s = va_arg(args, const char*);
                if (!s) s = "(null)";
                serial_puts(s);
                break;
            }
            case 'l': {
                p++; // Expect 'x' after 'l'
                if (*p == 'x') {
                    uint64_t val = va_arg(args, uint64_t);
                    serial_puts("0x");
                    kprint_hex(val);
                } else {
                    serial_putchar('%'); // Unknown format
                    serial_putchar('l');
                    serial_putchar(*p);
                }
                break;
            }
             case 'p': { // Pointer
                uintptr_t ptr = (uintptr_t)va_arg(args, void*);
                serial_puts("0x");
                kprint_hex((uint64_t)ptr);
                break;
            }
            case '%': {
                serial_putchar('%');
                break;
            }
            default:
                serial_putchar('%'); // Unknown format specifier
                serial_putchar(*p);
                break;
        }
    }
    va_end(args);
}

// --- Kernel Panic ---
static void kpanic(const char* msg, const char* file, int line) {
    // Disable interrupts FIRST
    asm volatile("cli");

    kprintf("\n*** KERNEL PANIC ***\n");
    kprintf("Reason: %s\n", msg);
    kprintf("At: %s:%d\n", file, line);

    // Add more debug info here if possible (registers, stack trace)

    // Halt forever
    kprintf("System halted.\n");
    while (1) {
        asm volatile("hlt");
    }
}

// Basic assertion macro using the improved kpanic
#define ASSERT(cond) \
    do { if (!(cond)) { kpanic("Assertion failed: " #cond, __FILE__, __LINE__); } } while(0)

// --- Optimized Memory Manipulation ---

// Optimized memset using rep stosb
void* memset(void* dest, int value, size_t count) {
    uint64_t d0; // Dummy output registers to inform compiler about clobbered regs
    asm volatile (
        "rep stosb"
        : "=&D"(d0), "+&c"(count) // Output: RDI (d0), RCX (count) modified
        : "a"(value), "D"(dest)   // Input: AL (value), RDI (dest)
        : "memory"                // Clobbers memory
    );
    return dest;
}

// Optimized memcpy using rep movsb
void* memcpy(void* dest, const void* src, size_t count) {
    uint64_t d0, s0; // Dummy output registers
    asm volatile (
        "rep movsb"
        : "=&D"(d0), "=&S"(s0), "+&c"(count) // Output: RDI, RSI, RCX modified
        : "D"(dest), "S"(src)               // Input: RDI, RSI
        : "memory"                         // Clobbers memory
    );
    return dest;
}


// --- Utility Functions ---

// Align value down/up
static inline uint64_t align_down(uint64_t val, uint64_t align) {
    return val & ~(align - 1);
}
static inline uint64_t align_up(uint64_t val, uint64_t align) {
    // Check if align is a power of 2 (optional but good practice)
    // ASSERT((align & (align - 1)) == 0);
    return align_down(val + align - 1, align);
}

// --- Synchronization (Basic Spinlock - unchanged) ---

typedef struct {
    volatile uint64_t locked;
} spinlock_t;

#define SPINLOCK_INIT {0}

static inline void spin_lock(spinlock_t* lock) {
    while (__atomic_test_and_set(&lock->locked, __ATOMIC_ACQUIRE)) {
        asm volatile("pause");
    }
}

static inline void spin_unlock(spinlock_t* lock) {
    __atomic_clear(&lock->locked, __ATOMIC_RELEASE);
}

// --- Physical Memory Manager (PMM) - Bitmap Allocator ---

static uint64_t* pmm_bitmap = NULL;
static uint64_t pmm_bitmap_size_qwords = 0; // Size in uint64_t entries
static uint64_t pmm_total_frames = 0;
static uint64_t pmm_used_frames = 0;
static uint64_t pmm_highest_frame = 0; // Highest usable frame index
static spinlock_t pmm_lock = SPINLOCK_INIT;
static uint64_t pmm_last_allocated_index = 0; // Hint for next allocation

// Global HHDM offset obtained from Limine
static uint64_t hhdm_phys_offset = 0;

// Converts physical address to virtual address in the HHDM
// NOW USES RUNTIME OFFSET
static inline void* phys_to_virt(uint64_t phys_addr) {
    ASSERT(hhdm_phys_offset != 0); // Ensure HHDM offset is initialized
    return (void*)(phys_addr + hhdm_phys_offset);
}

// Converts virtual address in the HHDM to physical address
// NOW USES RUNTIME OFFSET
static inline uint64_t virt_to_phys(void* virt_addr) {
    ASSERT(hhdm_phys_offset != 0); // Ensure HHDM offset is initialized
    ASSERT((uint64_t)virt_addr >= hhdm_phys_offset);
    return (uint64_t)virt_addr - hhdm_phys_offset;
}


// Sets a bit in the bitmap atomically
static inline void pmm_bitmap_set(uint64_t frame_index) {
    uint64_t idx = frame_index / 64;
    uint64_t bit = frame_index % 64;
    ASSERT(idx < pmm_bitmap_size_qwords);
    __atomic_fetch_or(&pmm_bitmap[idx], (1ull << bit), __ATOMIC_ACQ_REL);
}

// Clears a bit in the bitmap atomically
static inline void pmm_bitmap_clear(uint64_t frame_index) {
    uint64_t idx = frame_index / 64;
    uint64_t bit = frame_index % 64;
    ASSERT(idx < pmm_bitmap_size_qwords);
    __atomic_fetch_and(&pmm_bitmap[idx], ~(1ull << bit), __ATOMIC_ACQ_REL);
}

// Tests if a bit is set in the bitmap
static inline bool pmm_bitmap_test(uint64_t frame_index) {
    uint64_t idx = frame_index / 64;
    uint64_t bit = frame_index % 64;
    ASSERT(idx < pmm_bitmap_size_qwords);
    // Use relaxed atomic load as we only need atomicity relative to writes
    return (__atomic_load_n(&pmm_bitmap[idx], __ATOMIC_RELAXED) & (1ull << bit)) != 0;
}

// Finds the first 'count' contiguous free frames (Improved for count=1)
static int64_t pmm_find_first_free_contiguous(uint64_t count) {
    if (count == 0) return -1;
    if (count == 1) {
        // Optimized search for a single frame
        uint64_t start_qword = pmm_last_allocated_index / 64;
        for (uint64_t i = 0; i < pmm_bitmap_size_qwords; ++i) {
            uint64_t current_qword_idx = (start_qword + i) % pmm_bitmap_size_qwords;
            uint64_t qword = __atomic_load_n(&pmm_bitmap[current_qword_idx], __ATOMIC_RELAXED);

            if (qword != UINT64_MAX) { // If not all bits are set (i.e., at least one free frame)
                // Find the first zero bit (first free frame) in this qword
                // __builtin_ctzll counts trailing zeros. ~qword inverts bits, so ctz finds first '1' in inverted -> first '0' in original.
                uint64_t first_free_bit = __builtin_ctzll(~qword);
                uint64_t frame_index = current_qword_idx * 64 + first_free_bit;
                // Ensure the found frame is within the valid range
                if (frame_index <= pmm_highest_frame) {
                    return frame_index;
                }
                // If the found bit is outside the valid range, continue searching
                // (this might happen in the last qword if total_frames isn't a multiple of 64)
            }
        }
        return -1; // Not found
    } else {
        // Linear search for contiguous blocks (less optimized)
        uint64_t consecutive_free = 0;
        uint64_t start_index = pmm_last_allocated_index + 1;
        if (start_index > pmm_highest_frame) start_index = 0;

        for (uint64_t current_offset = 0; current_offset <= pmm_highest_frame; ++current_offset) {
            uint64_t search_index = (start_index + current_offset) % (pmm_highest_frame + 1);

            if (!pmm_bitmap_test(search_index)) {
                consecutive_free++;
                if (consecutive_free == count) {
                    return search_index - count + 1; // Found start of block
                }
            } else {
                consecutive_free = 0; // Reset counter
            }
        }
        return -1; // Not found
    }
}


// Allocates a single physical frame
uint64_t pmm_alloc_frame() {
    spin_lock(&pmm_lock);

    int64_t frame_index_signed = pmm_find_first_free_contiguous(1);

    if (frame_index_signed == -1) {
        // Try searching from the beginning if the hint didn't work (already done by find_first_free)
        spin_unlock(&pmm_lock);
        kpanic("PMM: Out of physical memory!", __FILE__, __LINE__);
        // Note: kpanic doesn't return
        return 0; // Should not be reached
    }

    uint64_t frame_index = (uint64_t)frame_index_signed;

    ASSERT(!pmm_bitmap_test(frame_index)); // Ensure it was actually free

    pmm_bitmap_set(frame_index);
    pmm_used_frames++;
    pmm_last_allocated_index = frame_index; // Update hint

    uint64_t phys_addr = frame_index * PAGE_SIZE;

    // Zero the allocated frame *before* releasing the lock
    void* virt_addr = phys_to_virt(phys_addr);
    memset(virt_addr, 0, PAGE_SIZE);

    spin_unlock(&pmm_lock);

    // kprintf("PMM: Allocated frame 0x%lx\n", phys_addr);
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
        kpanic("PMM: Attempted to free already free frame!", __FILE__, __LINE__);
        return; // Should not be reached
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
    spinlock_t lock; // Lock for modifying this specific address space
} address_space_t;

static address_space_t kernel_address_space;

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
// Returns the *virtual* address of the next level table
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
        // Set the entry: Present, Writable, User (maybe?), Frame Addr
        // User flag allows user access *to the table itself* if needed,
        // but actual page access depends on PTE flags. Keep kernel-only for now.
        // NX bit is implicitly 0 for table entries.
        // Ensure memset in pmm_alloc_frame zeroed the frame.
        *entry = frame | PTE_PRESENT | PTE_WRITE; // Add PTE_USER if tables should be user-accessible
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

    spin_lock(&space->lock);

    // Get virtual address of PML4
    pt_entry_t* pml4 = (pt_entry_t*)phys_to_virt(space->pml4_phys);

    // Walk the tables, allocating if necessary
    pt_entry_t* pdpt = vmm_get_next_level(&pml4[pml4_index], true);
    if (!pdpt) { spin_unlock(&space->lock); return false; }
    pt_entry_t* pd = vmm_get_next_level(&pdpt[pdpt_index], true);
    if (!pd) { spin_unlock(&space->lock); return false; }
    pt_entry_t* pt = vmm_get_next_level(&pd[pd_index], true);
    if (!pt) { spin_unlock(&space->lock); return false; }

    // Set the final Page Table Entry (PTE)
    pt_entry_t* pte = &pt[pt_index];
    if (*pte & PTE_PRESENT) {
        // Page already mapped - this might be an error or require unmapping first
        kprintf("VMM: Warning - Remapping page at 0x%lx (old phys 0x%lx, new phys 0x%lx)\n",
                virt_addr, *pte & PTE_ADDR_MASK, phys_addr);
        // Consider freeing the old frame if ownership is clear and this is not expected.
        // pmm_free_frame(*pte & PTE_ADDR_MASK); // Be careful with this!
    }

    *pte = (phys_addr & PTE_ADDR_MASK) | (flags & PTE_FLAGS_MASK) | PTE_PRESENT;

    spin_unlock(&space->lock);

    // Invalidate TLB for this address (outside the lock)
    invlpg(virt_addr_in);

    // kprintf("VMM: Mapped virt 0x%lx to phys 0x%lx flags 0x%lx\n", virt_addr, phys_addr, flags);
    return true;
}

// Checks if a page table (PT, PD, PDPT) is empty
static bool vmm_is_table_empty(pt_entry_t* table_virt) {
    for (int i = 0; i < 512; i++) {
        if (table_virt[i] & PTE_PRESENT) {
            return false; // Found a present entry
        }
    }
    return true; // All entries are non-present
}

// Unmaps a virtual page and potentially frees parent tables if they become empty.
// Returns the physical frame that was mapped, or 0 if not mapped.
// Does NOT free the returned physical frame. Caller must decide.
uint64_t vmm_unmap_page(address_space_t* space, void* virt_addr_in) {
    uint64_t virt_addr = (uint64_t)virt_addr_in;
    ASSERT((virt_addr % PAGE_SIZE) == 0);

    uint64_t pml4_index = (virt_addr >> 39) & 0x1FF;
    uint64_t pdpt_index = (virt_addr >> 30) & 0x1FF;
    uint64_t pd_index = (virt_addr >> 21) & 0x1FF;
    uint64_t pt_index = (virt_addr >> 12) & 0x1FF;

    spin_lock(&space->lock);

    pt_entry_t* pml4 = (pt_entry_t*)phys_to_virt(space->pml4_phys);
    pt_entry_t* pml4e = &pml4[pml4_index];
    if (!(*pml4e & PTE_PRESENT)) { spin_unlock(&space->lock); return 0; } // No PDPT

    pt_entry_t* pdpt = (pt_entry_t*)phys_to_virt(*pml4e & PTE_ADDR_MASK);
    pt_entry_t* pdpte = &pdpt[pdpt_index];
    if (!(*pdpte & PTE_PRESENT)) { spin_unlock(&space->lock); return 0; } // No PD

    pt_entry_t* pd = (pt_entry_t*)phys_to_virt(*pdpte & PTE_ADDR_MASK);
    pt_entry_t* pde = &pd[pd_index];
    if (!(*pde & PTE_PRESENT)) { spin_unlock(&space->lock); return 0; } // No PT

    pt_entry_t* pt = (pt_entry_t*)phys_to_virt(*pde & PTE_ADDR_MASK);
    pt_entry_t* pte = &pt[pt_index];
    if (!(*pte & PTE_PRESENT)) { spin_unlock(&space->lock); return 0; } // Page wasn't mapped

    // 1. Get physical frame and clear PTE
    uint64_t phys_frame = *pte & PTE_ADDR_MASK;
    *pte = 0; // Clear the entry

    // Invalidate TLB *after* modifying PTE but *before* potentially freeing tables
    invlpg(virt_addr_in);

    // 2. Check if PT is now empty
    if (vmm_is_table_empty(pt)) {
        uint64_t pt_phys = *pde & PTE_ADDR_MASK; // Get phys addr of PT from PDE
        *pde = 0; // Clear PDE pointing to the now-empty PT
        pmm_free_frame(pt_phys); // Free the PT's frame
        // kprintf("VMM: Freed empty PT at phys 0x%lx\n", pt_phys);

        // 3. Check if PD is now empty
        if (vmm_is_table_empty(pd)) {
            uint64_t pd_phys = *pdpte & PTE_ADDR_MASK; // Get phys addr of PD from PDPTE
            *pdpte = 0; // Clear PDPTE pointing to the now-empty PD
            pmm_free_frame(pd_phys); // Free the PD's frame
            // kprintf("VMM: Freed empty PD at phys 0x%lx\n", pd_phys);

            // 4. Check if PDPT is now empty
            if (vmm_is_table_empty(pdpt)) {
                uint64_t pdpt_phys = *pml4e & PTE_ADDR_MASK; // Get phys addr of PDPT from PML4E
                *pml4e = 0; // Clear PML4E pointing to the now-empty PDPT
                pmm_free_frame(pdpt_phys); // Free the PDPT's frame
                // kprintf("VMM: Freed empty PDPT at phys 0x%lx\n", pdpt_phys);
                // PML4 is never freed this way
            }
        }
    }

    spin_unlock(&space->lock);

    // kprintf("VMM: Unmapped virt 0x%lx (was phys 0x%lx)\n", virt_addr, phys_frame);
    return phys_frame;
}


// Switches the current address space
void vmm_switch_address_space(address_space_t* space) {
    // In a real scenario, you might need to handle TLB shootdowns
    // if switching between spaces that share kernel mappings modified
    // by other cores. For a single core or simple kernel, just loading
    // CR3 might suffice initially.
    load_cr3(space->pml4_phys);
    // kprintf("VMM: Switched address space to PML4 at 0x%lx\n", space->pml4_phys);
}

// --- Kernel Heap (kmalloc/kfree) - Simple Block Allocator ---

// Header size MUST be a multiple of alignment for the data pointer to be aligned
#define KHEAP_MIN_ALIGNMENT 16 // x86_64 requires 16-byte alignment for many operations
typedef struct kheap_block_header {
    size_t size; // Size of the data area *following* this header
    struct kheap_block_header* next_free;
    uint64_t magic; // For detecting corruption
    // Padding to make header size a multiple of KHEAP_MIN_ALIGNMENT (16)
    // sizeof(size_t)=8, sizeof(ptr)=8, sizeof(magic)=8 => 24 bytes. Need 8 bytes padding.
    uint64_t _padding;
} kheap_block_header_t;

#define KHEAP_MAGIC 0xDEADBEEFCAFEBABE
#define KHEAP_HEADER_SIZE sizeof(kheap_block_header_t) // Now 32 bytes (multiple of 16)

static kheap_block_header_t* kheap_free_list = NULL;
static uintptr_t kheap_current_break = KERNEL_HEAP_START;
static uintptr_t kheap_max_break = KERNEL_HEAP_START + KERNEL_HEAP_MAX_SIZE;
static spinlock_t kheap_lock = SPINLOCK_INIT;

// Adds a block to the free list (maintains sorted order by address)
static void kheap_add_to_free_list(kheap_block_header_t* block) {
    block->magic = KHEAP_MAGIC; // Set magic when freeing/adding
    block->next_free = NULL;

    // Ensure block address itself is aligned (should be if from pmm_alloc or split correctly)
    ASSERT(((uintptr_t)block % KHEAP_MIN_ALIGNMENT) == 0);

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
    // Align the *needed size* up to page size for expansion request
    uintptr_t expansion_size = align_up(bytes_needed, PAGE_SIZE);
    uintptr_t new_break = old_break + expansion_size;

    if (new_break > kheap_max_break) {
        kprintf("KHeap: Expansion failed - exceeds max heap size (req 0x%lx, current 0x%lx, new 0x%lx > max 0x%lx)\n",
                bytes_needed, old_break, new_break, kheap_max_break);
        return false;
    }

    // kprintf("KHeap: Expanding by 0x%lx bytes (from 0x%lx to 0x%lx)\n", expansion_size, old_break, new_break);

    for (uintptr_t addr = old_break; addr < new_break; addr += PAGE_SIZE) {
        uint64_t phys_frame = pmm_alloc_frame();
        if (phys_frame == 0) {
            kprintf("KHeap: Expansion failed - PMM out of memory during expansion\n");
            // Rollback? Difficult. Panic or return failure.
            // For simplicity, we don't rollback mappings here. A real kernel might try.
            kheap_current_break = addr; // Only update break to where we successfully mapped
            return false;
        }

        // Map heap pages as Read/Write, No-Execute
        if (!vmm_map_page(&kernel_address_space, (void*)addr, phys_frame, PTE_WRITE | PTE_NX)) {
            kprintf("KHeap: Expansion failed - VMM mapping error for virt 0x%lx -> phys 0x%lx\n", addr, phys_frame);
            pmm_free_frame(phys_frame); // Free the frame we couldn't map
            // Rollback? Difficult.
             kheap_current_break = addr; // Only update break to where we successfully mapped
            return false;
        }
    }
    kheap_current_break = new_break; // Expansion successful
    return true;
}

// Coalesces adjacent free blocks
static void kheap_coalesce_free_list() {
    kheap_block_header_t* current = kheap_free_list;
    while (current && current->next_free) {
        ASSERT(current->magic == KHEAP_MAGIC);
        ASSERT(current->next_free->magic == KHEAP_MAGIC);

        // Check if the end of the current block's data area touches the next free block's header
        if ((uintptr_t)current + KHEAP_HEADER_SIZE + current->size == (uintptr_t)current->next_free) {
            // Coalesce
            current->size += KHEAP_HEADER_SIZE + current->next_free->size; // Add size of next header + its data
            current->next_free = current->next_free->next_free; // Remove next block from list
            // Stay on current block to check if it can coalesce further
            // kprintf("KHeap: Coalesced block at %p, new data size %zu\n", current, current->size);
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

    // 1. Align desired data size
    size_t aligned_data_size = align_up(size, KHEAP_MIN_ALIGNMENT);
    // 2. Calculate total block size needed (header + aligned data)
    size_t total_block_size_needed = KHEAP_HEADER_SIZE + aligned_data_size;

    spin_lock(&kheap_lock);

    // 3. Search free list (first fit)
    kheap_block_header_t* current = kheap_free_list;
    kheap_block_header_t* prev = NULL;
    while (current) {
        ASSERT(current->magic == KHEAP_MAGIC); // Check for corruption

        // Does this free block have enough space for the *aligned data*?
        if (current->size >= aligned_data_size) {
            // Found a suitable block
            // Remove from free list
            if (prev) {
                prev->next_free = current->next_free;
            } else {
                kheap_free_list = current->next_free;
            }

            // Check if we can split the block.
            // Can we fit the requested data AND a new header AND minimum aligned data block?
            size_t remaining_size = current->size - aligned_data_size;
            if (remaining_size >= KHEAP_HEADER_SIZE + KHEAP_MIN_ALIGNMENT) {
                // Yes, split the block
                // Calculate address of the new free block's header
                kheap_block_header_t* split_free = (kheap_block_header_t*)((uintptr_t)current + KHEAP_HEADER_SIZE + aligned_data_size);
                split_free->size = remaining_size - KHEAP_HEADER_SIZE; // Data size for the new free block
                // Adjust size of the allocated block (data area size)
                current->size = aligned_data_size;
                // Add the split remainder back to the free list
                kheap_add_to_free_list(split_free); // Handles magic, sorting
                // kprintf("KHeap: Split block %p (orig data %zu) -> alloc %zu, free %p (data %zu)\n",
                //         current, current->size + remaining_size, current->size, split_free, split_free->size);
            }
            // Else: Use the whole block. current->size is already >= aligned_data_size.

            current->magic = ~KHEAP_MAGIC; // Mark as allocated (simple inversion)
            spin_unlock(&kheap_lock);

            void* data_ptr = (void*)((uintptr_t)current + KHEAP_HEADER_SIZE);
            // Verify alignment of returned pointer
            ASSERT(((uintptr_t)data_ptr % KHEAP_MIN_ALIGNMENT) == 0);
            // kprintf("KHeap: Allocated %zu bytes (aligned %zu) at %p (header %p)\n", size, aligned_data_size, data_ptr, current);
            return data_ptr;
        }
        prev = current;
        current = current->next_free;
    }

    // 4. No suitable block in free list, expand the heap
    // We need enough space for the total block size (header + aligned data)
    uintptr_t expansion_start = kheap_current_break;
    if (!kheap_expand(total_block_size_needed)) {
        // Expansion failed (already printed error in kheap_expand)
        spin_unlock(&kheap_lock);
        kprintf("KHeap: Allocation failed - cannot expand heap for %zu bytes\n", size);
        return NULL; // Out of memory
    }

    // Expansion successful, kheap_current_break is updated.
    // The newly expanded space starts at expansion_start and has size (kheap_current_break - expansion_start).
    size_t new_region_size = kheap_current_break - expansion_start;
    ASSERT(new_region_size >= total_block_size_needed);

    // Create a new block header at the beginning of the expanded space
    kheap_block_header_t* new_block = (kheap_block_header_t*)expansion_start;
    new_block->size = new_region_size - KHEAP_HEADER_SIZE; // Data size for the entire new region

    // Add this large new block to the free list so it can be potentially split/coalesced later
    kheap_add_to_free_list(new_block);

    // Coalesce immediately in case the new block is adjacent to a previous free block
    // (though unlikely with simple break pointer expansion)
    kheap_coalesce_free_list();

    // Retry allocation (recursive call, but safe due to expansion)
    // This simplifies logic as the new block is now on the free list and will be found.
    spin_unlock(&kheap_lock);
    // kprintf("KHeap: Expanded heap, retrying allocation for %zu bytes\n", size);
    return kmalloc(size); // Tail recursion might be optimized
}

// Free memory allocated by kmalloc
void kfree(void* ptr) {
    if (!ptr) {
        return;
    }

    // Calculate header address using the CORRECT (padded) header size
    kheap_block_header_t* block = (kheap_block_header_t*)((uintptr_t)ptr - KHEAP_HEADER_SIZE);

    spin_lock(&kheap_lock);

    // Basic sanity check - check if it *was* allocated (using inverted magic)
    ASSERT(block->magic == ~KHEAP_MAGIC);

    // Add block back to free list (will handle sorting and set magic back to KHEAP_MAGIC)
    kheap_add_to_free_list(block);

    // Coalesce free blocks
    kheap_coalesce_free_list();

    spin_unlock(&kheap_lock);
    // kprintf("KHeap: Freed block at %p (ptr %p, data size %zu)\n", block, ptr, block->size);
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
    // Initialize serial output first for debugging messages
    serial_init();
    kprintf("Kernel: Serial Initialized (COM1 @ 115200)\n");

    // --- Ensure Limine provided necessary info ---
    if (memmap_request.response == NULL) kpanic("Memory Init: Missing Limine memmap info!", __FILE__, __LINE__);
    if (hhdm_request.response == NULL) kpanic("Memory Init: Missing Limine HHDM info!", __FILE__, __LINE__);
    if (kernel_addr_request.response == NULL) kpanic("Memory Init: Missing Limine kernel address info!", __FILE__, __LINE__);

    struct limine_memmap_entry** memmap = memmap_request.response->entries;
    uint64_t memmap_entry_count = memmap_request.response->entry_count;
    hhdm_phys_offset = hhdm_request.response->offset; // Store the HHDM offset globally
    uint64_t kernel_phys_base = kernel_addr_request.response->physical_base;
    uint64_t kernel_virt_base = kernel_addr_request.response->virtual_base;

    kprintf("Memory Init: HHDM Offset: 0x%lx\n", hhdm_phys_offset);
    kprintf("Memory Init: Kernel Phys: 0x%lx, Kernel Virt: 0x%lx\n", kernel_phys_base, kernel_virt_base);
    // ASSERT(hhdm_phys_offset == HHDM_VMA_OFFSET); // Don't assert against hardcoded value anymore

    // --- Initialize Physical Memory Manager (PMM) ---
    kprintf("PMM: Initializing...\n");
    // 1. Find highest physical address and total usable memory
    uint64_t highest_addr = 0;
    uint64_t total_usable_mem = 0;
    for (uint64_t i = 0; i < memmap_entry_count; i++) {
        // kprintf("  MemMap [%lu]: base=0x%lx, len=0x%lx, type=%lu\n", i, memmap[i]->base, memmap[i]->length, memmap[i]->type);
        if (memmap[i]->type == LIMINE_MEMMAP_USABLE) {
            uint64_t top = memmap[i]->base + memmap[i]->length;
            total_usable_mem += memmap[i]->length;
            if (top > highest_addr) {
                highest_addr = top;
            }
        }
        // Consider highest address from *any* region for bitmap sizing
        uint64_t region_top = memmap[i]->base + memmap[i]->length;
        if (region_top > highest_addr) {
             highest_addr = region_top;
        }
    }
    // Ensure highest_addr is at least PAGE_SIZE aligned for frame calculation
    highest_addr = align_up(highest_addr, PAGE_SIZE);
    pmm_total_frames = highest_addr / PAGE_SIZE;
    pmm_highest_frame = pmm_total_frames - 1;
    // Calculate bitmap size in qwords (uint64_t)
    pmm_bitmap_size_qwords = align_up(pmm_total_frames, 64) / 64;
    uint64_t pmm_bitmap_size_bytes = pmm_bitmap_size_qwords * sizeof(uint64_t);

    kprintf("PMM: Highest Addr: 0x%lx, Total Frames: %lu, Bitmap Size: %lu bytes (%lu qwords)\n",
            highest_addr, pmm_total_frames, pmm_bitmap_size_bytes, pmm_bitmap_size_qwords);

    // 2. Find a place for the bitmap itself within usable physical memory
    uint64_t bitmap_phys_addr = 0;
    for (uint64_t i = 0; i < memmap_entry_count; i++) {
        // Find usable region large enough for the bitmap, preferably above 1MB
        if (memmap[i]->type == LIMINE_MEMMAP_USABLE && memmap[i]->length >= pmm_bitmap_size_bytes) {
            uint64_t potential_addr = memmap[i]->base;
            // Try to place it above 1MB to avoid potential conflicts with BIOS/low memory structures
            if (potential_addr < 0x100000) {
                 if (memmap[i]->base + memmap[i]->length > 0x100000) {
                     potential_addr = 0x100000; // Start search at 1MB if region crosses it
                 } else {
                     continue; // Region is entirely below 1MB, skip for bitmap placement
                 }
            }
            // Align the potential address up to a page boundary
            potential_addr = align_up(potential_addr, PAGE_SIZE);

            // Check if it still fits within the current usable region after alignment
            if (potential_addr + pmm_bitmap_size_bytes <= memmap[i]->base + memmap[i]->length) {
                 bitmap_phys_addr = potential_addr;
                 kprintf("PMM: Placing bitmap at phys 0x%lx (within region %lu)\n", bitmap_phys_addr, i);
                 break; // Found a spot
            }
        }
    }
    if (bitmap_phys_addr == 0) {
        kpanic("PMM: Could not find suitable space for the bitmap!", __FILE__, __LINE__);
    }

    // 3. Initialize the bitmap (mark all as used initially)
    pmm_bitmap = (uint64_t*)phys_to_virt(bitmap_phys_addr);
    memset(pmm_bitmap, 0xFF, pmm_bitmap_size_bytes); // Mark all bits as 1 (used)
    pmm_used_frames = pmm_total_frames; // Assume all used initially

    // 4. Mark usable regions as free in the bitmap
    for (uint64_t i = 0; i < memmap_entry_count; i++) {
        if (memmap[i]->type == LIMINE_MEMMAP_USABLE) {
            uint64_t start_frame = align_up(memmap[i]->base, PAGE_SIZE) / PAGE_SIZE;
            uint64_t end_frame = align_down(memmap[i]->base + memmap[i]->length, PAGE_SIZE) / PAGE_SIZE;

            if (end_frame > start_frame) {
                 // kprintf("PMM: Marking usable frames %lu to %lu as free\n", start_frame, end_frame - 1);
                 for (uint64_t frame = start_frame; frame < end_frame; frame++) {
                     if (pmm_bitmap_test(frame)) { // Check if currently marked used
                         pmm_bitmap_clear(frame);
                         ASSERT(pmm_used_frames > 0);
                         pmm_used_frames--;
                     }
                 }
            }
        }
    }

    // 5. Mark the bitmap area itself as used (critical!)
    uint64_t bitmap_start_frame = bitmap_phys_addr / PAGE_SIZE;
    uint64_t bitmap_end_frame = align_up(bitmap_phys_addr + pmm_bitmap_size_bytes, PAGE_SIZE) / PAGE_SIZE;
    kprintf("PMM: Marking bitmap frames %lu to %lu as used\n", bitmap_start_frame, bitmap_end_frame - 1);
    for (uint64_t frame = bitmap_start_frame; frame < bitmap_end_frame; frame++) {
        if (!pmm_bitmap_test(frame)) { // If it was marked free (because it was in a USABLE region)
            pmm_bitmap_set(frame);
            pmm_used_frames++;
        }
    }
    kprintf("PMM: Initialized. Used Frames: %lu, Free Frames: %lu\n",
            pmm_used_frames, pmm_total_frames - pmm_used_frames);


    // --- Initialize Virtual Memory Manager (VMM) ---
    kprintf("VMM: Initializing...\n");
    // 1. Get current PML4 (set up by Limine) and initialize kernel address space struct
    kernel_address_space.pml4_phys = read_cr3();
    kernel_address_space.lock = SPINLOCK_INIT; // Initialize spinlock
    kprintf("VMM: Initial kernel PML4 (from CR3) at phys 0x%lx\n", kernel_address_space.pml4_phys);

    // 2. Ensure HHDM is mapped correctly by iterating through physical memory
    // Limine should have done this, but we verify/extend and set correct flags.
    kprintf("VMM: Verifying/Mapping HHDM...\n");
    for (uint64_t i = 0; i < memmap_entry_count; ++i) {
        uint64_t base = memmap[i]->base;
        uint64_t top = base + memmap[i]->length;
        uint64_t flags = PTE_WRITE | PTE_NX; // Default: Kernel R/W, No-Execute data

        switch(memmap[i]->type) {
            case LIMINE_MEMMAP_USABLE:
            case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE:
            case LIMINE_MEMMAP_ACPI_RECLAIMABLE: // Might want to map these too
            case LIMINE_MEMMAP_ACPI_NVS:
                flags = PTE_WRITE | PTE_NX; // Standard data mapping
                break;

            case LIMINE_MEMMAP_KERNEL_AND_MODULES:
                // WARNING: Ideally, parse ELF sections. For now, map R/W/X.
                // This is simpler but less secure than R/X for code and R/W+NX for data.
                flags = PTE_WRITE; // Allow Write, No NX bit (Executable)
                kprintf("VMM: Mapping Kernel/Modules (0x%lx-0x%lx) as R/W/X\n", base, top);
                break;

            case LIMINE_MEMMAP_FRAMEBUFFER:
                // Framebuffers often need cache disabled or write-through
                flags = PTE_WRITE | PTE_NX | PTE_PCD | PTE_PWT;
                kprintf("VMM: Mapping Framebuffer (0x%lx-0x%lx) as R/W/NX + Cache Disabled\n", base, top);
                break;

            case LIMINE_MEMMAP_RESERVED: // Typically hardware MMIO - map carefully!
                 // Often requires cache disable, might be read-only or write-only
                 // flags = PTE_WRITE | PTE_NX | PTE_PCD | PTE_PWT; // Example: Map as uncached R/W
                 // kprintf("VMM: Mapping Reserved MMIO? (0x%lx-0x%lx) as R/W/NX + Cache Disabled\n", base, top);
                 // For safety, let's NOT map generic RESERVED by default here.
                 // Specific drivers should map the MMIO they need.
                 continue; // Skip mapping generic reserved regions in HHDM for now

            default:
                // Skip other types (BadMemory, etc.)
                continue;
        }

        // Map the physical range [base, top) to virtual [base+hhdm, top+hhdm)
        for (uint64_t paddr = align_down(base, PAGE_SIZE); paddr < top; paddr += PAGE_SIZE) {
            void* vaddr = phys_to_virt(paddr);
            // Map the page. If it fails, something is very wrong.
            if (!vmm_map_page(&kernel_address_space, vaddr, paddr, flags)) {
                 // Use kpanic directly here as VMM failure during init is fatal
                 kpanic("VMM: Failed to map HHDM page!", __FILE__, __LINE__);
            }
        }
    }
    kprintf("VMM: HHDM mapping complete.\n");

    // --- Initialize Kernel Heap ---
    // The virtual address range KERNEL_HEAP_START -> KERNEL_HEAP_START + KERNEL_HEAP_MAX_SIZE
    // is reserved, but pages are allocated and mapped on demand by kmalloc -> kheap_expand.
    kheap_free_list = NULL;
    kheap_current_break = KERNEL_HEAP_START; // Set initial break
    kheap_max_break = KERNEL_HEAP_START + KERNEL_HEAP_MAX_SIZE; // Set max break
    kheap_lock = SPINLOCK_INIT; // Initialize lock
    kprintf("KHeap: Initialized. Ready at virt 0x%lx (max size 0x%lx)\n",
            KERNEL_HEAP_START, KERNEL_HEAP_MAX_SIZE);

    kprintf("Memory Management Initialized Successfully.\n");
}
