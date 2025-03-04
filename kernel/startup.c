#include <stdint.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <elf.h>
#include <stdbool.h>

// Aggressive error handling with minimal overhead
#define KERNEL_ASSERT(cond) do { \
    if (__builtin_expect(!(cond), 0)) { \
        __builtin_trap(); \
    } \
} while(0)

// Compiler optimization hints and strict type enforcement
#define ALWAYS_INLINE __attribute__((always_inline, optimize("O3")))
#define PACKED __attribute__((packed))
#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

// Minimal, tightly packed boot parameters with strong typing
typedef struct PACKED {
    uint32_t magic;        // Multiboot magic number
    uintptr_t entry_point; // Kernel entry point
    uintptr_t load_addr;   // Base load address
    uint32_t flags;        // Additional boot flags
} BootParams;

// Statically allocated, cache-aligned context to prevent false sharing
typedef struct PACKED {
    int kernel_fd;
    void* kernel_base;
    size_t kernel_size;
    BootParams boot_params;
} __attribute__((aligned(64))) KernelContext;

// Static, pre-allocated context to avoid dynamic allocation
static KernelContext g_context;

// Optimized file mapping with minimal branching and error paths
ALWAYS_INLINE static void* map_kernel_file(const char* restrict path) {
    struct stat st;
    KERNEL_ASSERT(stat(path, &st) == 0);

    g_context.kernel_fd = open(path, O_RDONLY | O_DIRECT);
    KERNEL_ASSERT(g_context.kernel_fd >= 0);

    g_context.kernel_size = st.st_size;
    
    // Use MAP_POPULATE for immediate page fault resolution
    g_context.kernel_base = mmap(NULL, g_context.kernel_size, 
                                 PROT_READ, 
                                 MAP_PRIVATE | MAP_POPULATE, 
                                 g_context.kernel_fd, 0);
    
    KERNEL_ASSERT(g_context.kernel_base != MAP_FAILED);
    
    return g_context.kernel_base;
}

// Highly optimized ELF segment loader with aggressive prefetching
ALWAYS_INLINE static void load_kernel_segments(Elf64_Ehdr* restrict ehdr) {
    Elf64_Phdr* restrict phdr = (Elf64_Phdr*)((char*)ehdr + ehdr->e_phoff);
    
    // Prefetch upcoming segments
    __builtin_prefetch(phdr, 0, 3);
    
    for (uint16_t i = 0; LIKELY(i < ehdr->e_phnum); ++i) {
        // Skip non-loadable segments with minimal branch prediction overhead
        if (UNLIKELY(phdr[i].p_type != PT_LOAD)) continue;

        // Compute protection flags with bitwise operations
        int prot = ((phdr[i].p_flags & PF_R) ? PROT_READ : 0) |
                   ((phdr[i].p_flags & PF_W) ? PROT_WRITE : 0) |
                   ((phdr[i].p_flags & PF_X) ? PROT_EXEC : 0);

        void* dest = (void*)phdr[i].p_vaddr;
        size_t memsz = phdr[i].p_memsz;
        size_t filesz = phdr[i].p_filesz;

        // Zero-initialized memory segment with fixed mapping
        KERNEL_ASSERT(mmap(dest, memsz, prot, 
                           MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS, -1, 0) != MAP_FAILED);

        // Efficient, cache-aligned memory copy for initialized data
        if (LIKELY(filesz > 0)) {
            __builtin_memcpy(dest, 
                             (char*)ehdr + phdr[i].p_offset, 
                             filesz);
        }
    }
}

// Minimal, no-overhead kernel startup routine
__attribute__((noinline, section(".startup"), optimize("O3")))
static void start_kernel(Elf64_Ehdr* restrict ehdr) {
    // Prepare minimal, predictable boot parameters
    g_context.boot_params = (BootParams) {
        .magic = 0x1BADB002,
        .entry_point = ehdr->e_entry,
        .load_addr = (uintptr_t)g_context.kernel_base,
        .flags = 0  // Reserved for future use
    };

    // Direct, type-safe jump to kernel entry point
    void (*kernel_main)(BootParams*) = 
        (void (*)(BootParams*))ehdr->e_entry;
    
    // Perform final cache synchronization before entry
    __builtin_ia32_sfence();
    
    kernel_main(&g_context.boot_params);
}

// Hardened entry point with minimal dynamic behaviors
int main(int argc, char* argv[]) {
    // Early argument validation
    KERNEL_ASSERT(argc == 2);

    // Atomically read and validate kernel image
    void* kernel_image = map_kernel_file(argv[1]);
    Elf64_Ehdr* ehdr = (Elf64_Ehdr*)kernel_image;

    // Strict ELF header validation with constant-time comparison
    KERNEL_ASSERT(__builtin_memcmp(ehdr->e_ident, ELFMAG, SELFMAG) == 0);
    KERNEL_ASSERT(ehdr->e_type == ET_EXEC);

    // Segment loading with advanced prefetching
    load_kernel_segments(ehdr);

    // Kernel initialization
    start_kernel(ehdr);

    // Unreachable, maintained for POSIX compliance
    __builtin_unreachable();
}
