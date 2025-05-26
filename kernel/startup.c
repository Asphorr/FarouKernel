/**
 * @file kernel_loader.c
 * @brief A robust and optimized ELF kernel loader.
 *
 * This program loads a 64-bit ELF executable file (presumably a kernel)
 * into memory according to its program headers and then jumps to its
 * entry point. It prioritizes correctness, security, and performance.
 */

#define _GNU_SOURCE // For O_DIRECT, MAP_POPULATE if needed, though O_DIRECT is removed.
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h> // For memcpy, memcmp
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <elf.h>
#include <stddef.h> // For size_t, offsetof
#include <inttypes.h> // For PRIxPTR, PRIu64, etc.

// --- Configuration ---

// Define the expected ELF machine type (e.g., EM_X86_64).
// Set to EM_NONE to skip machine check (less safe).
#define EXPECTED_ELF_MACHINE EM_X86_64

// Define the expected ELF class (ELFCLASS64 or ELFCLASS32).
#define EXPECTED_ELF_CLASS ELFCLASS64

// --- Error Handling ---

/**
 * @brief Prints an error message to stderr and exits.
 *
 * @param func_name The name of the function where the error occurred.
 * @param msg A descriptive error message.
 * @param show_errno If true, appends the system error message (from errno).
 */
static void fail(const char *func_name, const char *msg, bool show_errno) {
    fprintf(stderr, "ERROR [%s]: %s", func_name, msg);
    if (show_errno && errno != 0) {
        fprintf(stderr, " (%s)", strerror(errno));
    }
    fprintf(stderr, "\n");
    // Consider more specific exit codes
    exit(EXIT_FAILURE);
}

// --- Utility Macros ---

// Branch prediction hints (use standard C likely/unlikely if available in C23+)
#define LIKELY(x)   __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

// --- Data Structures ---

// Boot parameters passed to the kernel.
// Removed PACKED: Standard alignment is usually fine and safer/faster.
// Ensure the kernel expects this layout.
typedef struct {
    uint32_t magic;        // Multiboot magic number (example)
    uintptr_t entry_point; // Kernel entry point address
    uintptr_t load_addr;   // Base address where the kernel *file* was mapped
    uint32_t flags;        // Additional boot flags
    uint64_t kernel_file_size; // Size of the original kernel file
} BootParams;

// Context structure holding loader state.
// Removed PACKED. Cache alignment is good.
typedef struct {
    int kernel_fd;          // File descriptor for the kernel image
    void *kernel_map_base;  // Base address of the mmap'd kernel file
    size_t kernel_map_size; // Size of the mmap'd kernel file
    BootParams boot_params; // Parameters to pass to the kernel
} __attribute__((aligned(64))) KernelContext;

// Static context to avoid dynamic allocation in this simple loader.
// Zero-initialized by default.
static KernelContext g_context;

// --- Resource Management ---

// Cleanup function for the file descriptor
static void cleanup_fd(int *fd) {
    if (fd && *fd >= 0) {
        if (close(*fd) == -1) {
            // Log error, but don't exit during cleanup if possible
            perror("Warning [cleanup_fd]: close failed");
        }
        *fd = -1;
    }
}

// Cleanup function for the memory mapping
static void cleanup_mmap(void **addr, size_t *size) {
    if (addr && *addr != MAP_FAILED && size && *size > 0) {
        if (munmap(*addr, *size) == -1) {
            // Log error, but don't exit during cleanup if possible
            perror("Warning [cleanup_mmap]: munmap failed");
        }
        *addr = MAP_FAILED;
        *size = 0;
    }
}

// --- Core Logic ---

/**
 * @brief Maps the kernel file into memory.
 *
 * Reads the file specified by path, maps it into read-only memory.
 *
 * @param path Path to the kernel ELF file.
 * @return Pointer to the mapped memory region. Exits on failure.
 */
static void *map_kernel_file(const char * restrict path) {
    struct stat st;

    // Reset errno before system calls
    errno = 0;
    if (stat(path, &st) == -1) {
        fail("map_kernel_file (stat)", "Failed to stat kernel file", true);
    }

    if (st.st_size <= 0) {
        fail("map_kernel_file (stat)", "Kernel file is empty or invalid size", false);
    }
    // Check for excessively large files (potential DoS)
    if ((uintmax_t)st.st_size > SIZE_MAX) {
         fail("map_kernel_file (stat)", "Kernel file size exceeds SIZE_MAX", false);
    }
    g_context.kernel_map_size = (size_t)st.st_size;

    // Use GCC cleanup attribute for automatic closing on scope exit
    // Note: Requires g_context.kernel_fd to be initialized to -1 or similar
    // if map_kernel_file can fail before open(). Let's initialize explicitly.
    g_context.kernel_fd = -1;
    // int kernel_fd __attribute__((__cleanup__(cleanup_fd))) = -1; // Alternative if g_context wasn't used

    errno = 0;
    // Removed O_DIRECT: Often hurts performance for full file reads, adds complexity.
    // Rely on the OS page cache.
    g_context.kernel_fd = open(path, O_RDONLY | O_CLOEXEC);
    if (g_context.kernel_fd < 0) {
        fail("map_kernel_file (open)", "Failed to open kernel file", true);
    }

    errno = 0;
    // MAP_PRIVATE: Changes are not written back to the file.
    // MAP_POPULATE: Hint to pre-populate page tables (can increase startup time
    //               but reduce initial page faults). Keep it as it was intended.
    // PROT_READ: Initially map only for reading. Segments will get specific perms later.
    g_context.kernel_map_base = mmap(NULL, // Let the kernel choose the address
                                     g_context.kernel_map_size,
                                     PROT_READ,
                                     MAP_PRIVATE | MAP_POPULATE,
                                     g_context.kernel_fd,
                                     0); // Offset 0

    if (g_context.kernel_map_base == MAP_FAILED) {
        cleanup_fd(&g_context.kernel_fd); // Manually clean up fd on mmap failure
        fail("map_kernel_file (mmap)", "Failed to map kernel file", true);
    }

    // File descriptor is no longer needed after mmap succeeds with MAP_PRIVATE.
    // Closing it now simplifies cleanup later.
    cleanup_fd(&g_context.kernel_fd);

    return g_context.kernel_map_base;
}

/**
 * @brief Validates the ELF header.
 *
 * Checks magic number, class, machine type, and basic header integrity.
 *
 * @param ehdr Pointer to the Elf64_Ehdr structure.
 * @param file_size The total size of the mapped file for bounds checking.
 */
static void validate_elf_header(const Elf64_Ehdr * restrict ehdr, size_t file_size) {
    // 1. Check Magic Number
    if (memcmp(ehdr->e_ident, ELFMAG, SELFMAG) != 0) {
        fail("validate_elf_header", "Invalid ELF magic number", false);
    }

    // 2. Check ELF Class (32/64 bit)
    if (ehdr->e_ident[EI_CLASS] != EXPECTED_ELF_CLASS) {
        fail("validate_elf_header", "Incorrect ELF class (expected 64-bit)", false);
    }

    // 3. Check Endianness (optional, assumes loader matches kernel)
    if (ehdr->e_ident[EI_DATA] != ELFDATA2LSB && ehdr->e_ident[EI_DATA] != ELFDATA2MSB) {
        fail("validate_elf_header", "Invalid ELF data encoding", false);
    }
    // Could add a check against host endianness if needed.

    // 4. Check ELF Version
    if (ehdr->e_ident[EI_VERSION] != EV_CURRENT) {
        fail("validate_elf_header", "Invalid ELF version", false);
    }

    // 5. Check OS ABI (optional, could check for ELFOSABI_SYSV or ELFOSABI_LINUX)
    // if (ehdr->e_ident[EI_OSABI] != ELFOSABI_SYSV && ehdr->e_ident[EI_OSABI] != ELFOSABI_NONE) { ... }

    // 6. Check File Type (must be executable)
    if (ehdr->e_type != ET_EXEC) {
        fail("validate_elf_header", "ELF file is not an executable type", false);
    }

    // 7. Check Machine Architecture (important for correctness)
#if EXPECTED_ELF_MACHINE != EM_NONE
    if (ehdr->e_machine != EXPECTED_ELF_MACHINE) {
        fail("validate_elf_header", "Incorrect ELF machine architecture", false);
    }
#endif

    // 8. Check Header Sizes and Counts for basic sanity and potential overflows
    if (ehdr->e_ehsize < sizeof(Elf64_Ehdr)) {
        fail("validate_elf_header", "Invalid ELF header size in header", false);
    }
    if (ehdr->e_phentsize < sizeof(Elf64_Phdr)) {
        // Allow larger entries, but not smaller
        fail("validate_elf_header", "Invalid Program Header entry size in header", false);
    }
    if (ehdr->e_phnum == 0) {
        fail("validate_elf_header", "No program headers found", false);
    }
    // Check if program header table offset is within file bounds
    if (ehdr->e_phoff == 0 || ehdr->e_phoff > file_size) {
         fail("validate_elf_header", "Program header table offset out of bounds", false);
    }

    // Check if the entire program header table fits within the file
    // Check for potential integer overflow before calculating end offset
    size_t ph_table_size;
    if (__builtin_mul_overflow(ehdr->e_phnum, ehdr->e_phentsize, &ph_table_size)) {
        fail("validate_elf_header", "Integer overflow calculating program header table size", false);
    }
    size_t ph_table_end;
    if (__builtin_add_overflow(ehdr->e_phoff, ph_table_size, &ph_table_end)) {
        fail("validate_elf_header", "Integer overflow calculating program header table end offset", false);
    }
    if (ph_table_end > file_size) {
        fail("validate_elf_header", "Program header table extends beyond file end", false);
    }

    // Check entry point validity (basic check: not NULL)
    if (ehdr->e_entry == 0) {
        fail("validate_elf_header", "ELF entry point is NULL", false);
    }
}


/**
 * @brief Loads ELF program segments into memory.
 *
 * Iterates through program headers, maps memory for PT_LOAD segments
 * at their specified virtual addresses using MAP_FIXED, and copies
 * data from the kernel file image.
 *
 * @param ehdr Pointer to the validated Elf64_Ehdr structure.
 */
static void load_kernel_segments(const Elf64_Ehdr * restrict ehdr) {
    // Pointer to the start of the program header table
    const Elf64_Phdr *phdr_table = (const Elf64_Phdr *)((const char *)ehdr + ehdr->e_phoff);

    // Prefetching can sometimes help, but profile to confirm benefit.
    // Prefetch the first cache line of the program header table.
    __builtin_prefetch(phdr_table, 0 /* read */, 3 /* high locality */);

    for (uint16_t i = 0; i < ehdr->e_phnum; ++i) {
        // Use pointer arithmetic for clarity, ensure correct stride if e_phentsize > sizeof(Elf64_Phdr)
        const Elf64_Phdr *phdr = (const Elf64_Phdr *)((const char *)phdr_table + i * ehdr->e_phentsize);

        // Prefetch the *next* program header entry while processing the current one.
        if (LIKELY(i + 1 < ehdr->e_phnum)) {
             const Elf64_Phdr *next_phdr = (const Elf64_Phdr *)((const char *)phdr_table + (i + 1) * ehdr->e_phentsize);
            __builtin_prefetch(next_phdr, 0, 3);
        }

        // Process only loadable segments
        if (phdr->p_type != PT_LOAD) {
            continue;
        }

        // --- Segment Validation ---
        // Check segment file bounds
        if (phdr->p_offset > g_context.kernel_map_size) {
            fprintf(stderr, "ERROR [load_kernel_segments]: Segment %u offset (0x%" PRIx64 ") out of file bounds (0x%zx)\n",
                    i, phdr->p_offset, g_context.kernel_map_size);
            exit(EXIT_FAILURE);
        }
        size_t segment_file_end;
        if (__builtin_add_overflow(phdr->p_offset, phdr->p_filesz, &segment_file_end)) {
             fprintf(stderr, "ERROR [load_kernel_segments]: Integer overflow calculating segment %u file end\n", i);
             exit(EXIT_FAILURE);
        }
        if (segment_file_end > g_context.kernel_map_size) {
             fprintf(stderr, "ERROR [load_kernel_segments]: Segment %u file data (offset 0x%" PRIx64 ", size 0x%" PRIx64 ") extends beyond file end (0x%zx)\n",
                    i, phdr->p_offset, phdr->p_filesz, g_context.kernel_map_size);
             exit(EXIT_FAILURE);
        }

        // Check segment memory size (p_memsz >= p_filesz)
        if (phdr->p_memsz < phdr->p_filesz) {
            fprintf(stderr, "ERROR [load_kernel_segments]: Segment %u memory size (0x%" PRIx64 ") is less than file size (0x%" PRIx64 ")\n",
                    i, phdr->p_memsz, phdr->p_filesz);
            exit(EXIT_FAILURE);
        }

        // Check for zero memory size (technically allowed, but unusual for PT_LOAD)
        if (phdr->p_memsz == 0) {
            fprintf(stderr, "Warning [load_kernel_segments]: Segment %u has zero memory size, skipping.\n", i);
            continue;
        }

        // Check virtual address alignment (required by mmap MAP_FIXED)
        // Although mmap might round down, ELF requires p_vaddr to be congruent
        // to p_offset modulo the page size. Let's check p_vaddr alignment itself.
        long page_size = sysconf(_SC_PAGESIZE);
        if (page_size <= 0) {
            fail("load_kernel_segments", "Failed to get system page size", true);
        }
        if (phdr->p_vaddr % (uint64_t)page_size != 0) {
             fprintf(stderr, "ERROR [load_kernel_segments]: Segment %u virtual address (0x%" PRIx64 ") is not page aligned (page size %ld)\n",
                    i, phdr->p_vaddr, page_size);
             exit(EXIT_FAILURE);
        }

        // Check for potential address space exhaustion or wrap-around
        uintptr_t segment_mem_end;
        if (__builtin_add_overflow(phdr->p_vaddr, phdr->p_memsz, &segment_mem_end)) {
             fprintf(stderr, "ERROR [load_kernel_segments]: Integer overflow calculating segment %u memory end address (vaddr 0x%" PRIx64 ", size 0x%" PRIx64 ")\n",
                    i, phdr->p_vaddr, phdr->p_memsz);
             exit(EXIT_FAILURE);
        }
        // Basic check against obviously invalid addresses (e.g., mapping over NULL page)
        if (phdr->p_vaddr < (uint64_t)page_size) {
             fprintf(stderr, "Warning [load_kernel_segments]: Segment %u virtual address (0x%" PRIx64 ") is very low, potential conflict.\n", i, phdr->p_vaddr);
             // Allow, but warn. A real bootloader might reserve low memory.
        }

        // --- Segment Mapping ---
        // Calculate protection flags
        int prot = PROT_NONE; // Start with no permissions
        if (phdr->p_flags & PF_R) prot |= PROT_READ;
        if (phdr->p_flags & PF_W) prot |= PROT_WRITE;
        if (phdr->p_flags & PF_X) prot |= PROT_EXEC;

        // WARNING: MAP_FIXED is dangerous! It overwrites existing mappings.
        // This is often necessary for loading a non-relocatable kernel, but
        // assumes the kernel's load addresses don't conflict with the loader itself.
        // Consider MAP_FIXED_NOREPLACE (Linux 4.17+) for a safer alternative if overlap is unacceptable.
        // Here, we stick to MAP_FIXED as implied by the original code's intent.
        void *dest = (void *)(uintptr_t)phdr->p_vaddr;
        size_t map_size = phdr->p_memsz;

        errno = 0;
        void *mapped_addr = mmap(dest,
                                 map_size,
                                 prot | PROT_WRITE, // Temporarily add PROT_WRITE for memcpy
                                 MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED,
                                 -1, // Anonymous mapping
                                 0);

        if (mapped_addr == MAP_FAILED) {
            fprintf(stderr, "ERROR [load_kernel_segments]: Failed to mmap segment %u at address %p (size %zu)",
                    i, dest, map_size);
            if (errno != 0) fprintf(stderr, " (%s)", strerror(errno));
            fprintf(stderr, "\n");
            // Consider attempting to unmap previously mapped segments on failure? Complex.
            exit(EXIT_FAILURE);
        }
        if (mapped_addr != dest) {
            // Should not happen with MAP_FIXED unless error, but double-check.
            fprintf(stderr, "ERROR [load_kernel_segments]: MAP_FIXED returned %p instead of requested %p for segment %u\n",
                    mapped_addr, dest, i);
            munmap(mapped_addr, map_size); // Attempt cleanup
            exit(EXIT_FAILURE);
        }

        // --- Data Copying ---
        if (phdr->p_filesz > 0) {
            // Source data from the initially mapped kernel file
            const void *src = (const char *)ehdr + phdr->p_offset;
            memcpy(dest, src, phdr->p_filesz);
        }

        // --- Final Permissions ---
        // If the segment should not be writable, remove PROT_WRITE.
        // This helps enforce W^X (Write XOR Execute).
        if (!(phdr->p_flags & PF_W)) {
            errno = 0;
            if (mprotect(dest, map_size, prot) == -1) {
                fprintf(stderr, "Warning [load_kernel_segments]: Failed to mprotect segment %u at %p to remove write permission (%s)\n",
                        i, dest, strerror(errno));
                // Continue, but security is reduced. Could choose to exit instead.
            }
        }

        // Note: The BSS section (difference between p_memsz and p_filesz)
        // is automatically zero-filled by MAP_ANONYMOUS.
    }
}

/**
 * @brief Prepares parameters and jumps to the kernel entry point.
 *
 * Marked noinline as it's a transition point.
 * Section attribute might be needed for specific bootloader scenarios.
 *
 * @param ehdr Pointer to the validated Elf64_Ehdr structure.
 */
__attribute__((noinline /*, section(".startup")*/ )) // Keep noinline, section optional
static void start_kernel(const Elf64_Ehdr * restrict ehdr) {

    // Define the kernel entry point function signature
    // Assumes kernel expects a pointer to BootParams. Adjust if different.
    typedef void (*KernelEntryFunc)(BootParams *);

    // Prepare boot parameters
    g_context.boot_params = (BootParams) {
        .magic = 0x1BADB002, // Example Multiboot magic
        .entry_point = (uintptr_t)ehdr->e_entry,
        .load_addr = (uintptr_t)g_context.kernel_map_base, // Base address of original file map
        .flags = 0, // Reserved
        .kernel_file_size = g_context.kernel_map_size
    };

    KernelEntryFunc kernel_main = (KernelEntryFunc)(uintptr_t)ehdr->e_entry;

    // --- Cache Coherency ---
    // Before jumping to code that we might have just written (via memcpy),
    // ensure instruction cache coherency.
    // __builtin_ia32_sfence() is a *store* fence (ensures stores are visible).
    // It does NOT guarantee instruction cache coherency on x86.
    // Modern x86 processors generally handle I-cache coherency automatically
    // for self-modifying code *if* writes and instruction fetches use the
    // same linear address. This is the case here.
    // Explicit I-cache flushing (like `clflush` loops) is complex and often unnecessary.
    // Some sources suggest a serializing instruction like `cpuid` can help,
    // but Intel manuals state it's mainly for cross-modifying code scenarios.
    // For simplicity and relying on modern x86 behavior, we can omit explicit flushing.
    // If targeting architectures *without* automatic I-cache coherency (like some ARM),
    // specific cache maintenance operations (e.g., using __builtin___clear_cache or asm)
    // would be *required* here.

    // __builtin_ia32_sfence(); // Removed: Not the correct barrier for I-cache.
    // Alternative (might be overkill, profile if needed):
    // asm volatile ("cpuid" ::: "eax", "ebx", "ecx", "edx", "memory");

    // Optional: Full memory barrier if unsure about ordering requirements before jump.
    // asm volatile ("mfence" ::: "memory");

    // Clean up the initial mapping of the kernel file *before* jumping.
    // We pass the base address and size in boot_params if the kernel needs them.
    // Do NOT use the cleanup_mmap function directly here, as it might log errors
    // via perror after we potentially lose stderr.
    if (g_context.kernel_map_base != MAP_FAILED && g_context.kernel
