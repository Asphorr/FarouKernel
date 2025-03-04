#include <stdint.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <elf.h>

// Inline error handling to reduce function call overhead
#define HANDLE_ERROR(msg) do { \
    __builtin_trap(); \
} while(0)

// Minimal boot parameters structure
typedef struct __attribute__((packed)) {
    uint32_t magic;
    uintptr_t entry_point;
    uintptr_t load_addr;
} BootParams;

// Statically allocated minimal context
static struct {
    int kernel_fd;
    void* kernel_base;
    size_t kernel_size;
    BootParams boot_params;
} g_context;

// Optimized file mapping with minimal error checking
static inline void* map_kernel_file(const char* path) {
    struct stat st;
    if (stat(path, &st) < 0) HANDLE_ERROR("stat failed");

    g_context.kernel_fd = open(path, O_RDONLY);
    if (g_context.kernel_fd < 0) HANDLE_ERROR("open failed");

    g_context.kernel_size = st.st_size;
    g_context.kernel_base = mmap(NULL, g_context.kernel_size, 
                                 PROT_READ, MAP_PRIVATE, 
                                 g_context.kernel_fd, 0);
    
    if (g_context.kernel_base == MAP_FAILED) HANDLE_ERROR("mmap failed");

    return g_context.kernel_base;
}

// Highly optimized ELF segment loader
static inline void load_kernel_segments(Elf32_Ehdr* ehdr) {
    Elf32_Phdr* phdr = (Elf32_Phdr*)((char*)ehdr + ehdr->e_phoff);
    
    for (uint16_t i = 0; i < ehdr->e_phnum; ++i) {
        if (phdr[i].p_type != PT_LOAD) continue;

        // Minimal protection mapping
        int prot = ((phdr[i].p_flags & PF_R) ? PROT_READ : 0) |
                   ((phdr[i].p_flags & PF_W) ? PROT_WRITE : 0) |
                   ((phdr[i].p_flags & PF_X) ? PROT_EXEC : 0);

        void* dest = (void*)phdr[i].p_vaddr;
        size_t memsz = phdr[i].p_memsz;
        size_t filesz = phdr[i].p_filesz;

        // Zero-initialize full memory segment
        if (mmap(dest, memsz, prot, 
                 MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS, -1, 0) == MAP_FAILED) {
            HANDLE_ERROR("segment mapping failed");
        }

        // Direct memory copy for initialized data
        if (filesz > 0) {
            __builtin_memcpy(dest, 
                             (char*)ehdr + phdr[i].p_offset, 
                             filesz);
        }
    }
}

// Minimal kernel startup routine
__attribute__((noinline, section(".startup")))
void start_kernel(Elf32_Ehdr* ehdr) {
    // Prepare minimal boot parameters
    g_context.boot_params = (BootParams) {
        .magic = 0x1BADB002,
        .entry_point = ehdr->e_entry,
        .load_addr = (uintptr_t)g_context.kernel_base
    };

    // Direct jump to kernel entry point
    void (*kernel_main)(BootParams*) = 
        (void (*)(BootParams*))ehdr->e_entry;
    
    kernel_main(&g_context.boot_params);
}

// Minimalist main function
int main(int argc, char* argv[]) {
    if (argc != 2) __builtin_trap();

    void* kernel_image = map_kernel_file(argv[1]);
    Elf32_Ehdr* ehdr = (Elf32_Ehdr*)kernel_image;

    // Validate ELF header with minimal checks
    if (*(uint32_t*)ehdr->e_ident != *(uint32_t*)ELFMAG) __builtin_trap();

    load_kernel_segments(ehdr);
    start_kernel(ehdr);

    // Unreachable, but kept for compliance
    return 0;
}
