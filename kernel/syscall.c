#include <stdint.h>
#include <stddef.h>
#include "syscalls.h"

//==================================================================
// CONFIGURATION AND MACROS
//==================================================================

#define SYSCALL_INLINE __attribute__((always_inline, flatten)) static inline
#define LIKELY(x)      __builtin_expect(!!(x), 1)
#define UNLIKELY(x)    __builtin_expect(!!(x), 0)
#define UNUSED(x)      ((void)(x))

// Cache line alignment for better performance
#define CACHE_LINE_SIZE 64
#define CACHE_ALIGNED   __attribute__((aligned(CACHE_LINE_SIZE)))

//==================================================================
// KERNEL LOGGING INTERFACE
//==================================================================

// Weak symbol allows override by actual kernel implementation
__attribute__((weak))
void kernel_log_impl(const char* func, const char* msg) {
    UNUSED(func);
    UNUSED(msg);
    // Real implementation should call printk() or equivalent
}

#ifdef DEBUG_SYSCALLS
    #define KERNEL_LOG(fn, msg) kernel_log_impl(fn, msg)
#else
    #define KERNEL_LOG(fn, msg) ((void)0)
#endif

//==================================================================
// SYSCALL ARGUMENT STRUCTURE
//==================================================================

typedef struct {
    union {
        struct { 
            uint64_t arg0, arg1, arg2, arg3, arg4, arg5; 
        };
        uint64_t args[6];
    };
} syscall_args_t;

//==================================================================
// SYSCALL HANDLER TYPE AND FORWARD DECLARATIONS
//==================================================================

typedef uint64_t (*syscall_handler_t)(const syscall_args_t*);

// Forward declarations
SYSCALL_INLINE uint64_t sys_write(const syscall_args_t *a);
SYSCALL_INLINE uint64_t sys_read(const syscall_args_t *a);
SYSCALL_INLINE uint64_t sys_open(const syscall_args_t *a);
SYSCALL_INLINE uint64_t sys_close(const syscall_args_t *a);
SYSCALL_INLINE uint64_t sys_lseek(const syscall_args_t *a);
SYSCALL_INLINE uint64_t sys_fstat(const syscall_args_t *a);
SYSCALL_INLINE uint64_t sys_exit(const syscall_args_t *a);

//==================================================================
// SYSCALL TABLE
//==================================================================

// Use designated initializers for clarity and safety
static const syscall_handler_t syscall_table[SYS_MAX] CACHE_ALIGNED = {
    [SYS_WRITE] = sys_write,
    [SYS_READ]  = sys_read,
    [SYS_OPEN]  = sys_open,
    [SYS_CLOSE] = sys_close,
    [SYS_LSEEK] = sys_lseek,
    [SYS_FSTAT] = sys_fstat,
    [SYS_EXIT]  = sys_exit,
    // All other entries are implicitly NULL
};

//==================================================================
// SYSCALL ENTRY POINT
//==================================================================

/**
 * @brief Main syscall dispatcher entry point
 * 
 * @param num Syscall number (from RAX register)
 * @param args Pointer to argument structure
 * @return Result of syscall or -errno on error
 */
uint64_t syscall_entry(uint64_t num, const syscall_args_t *args) {
    // Bounds check with single comparison
    if (UNLIKELY(num >= SYS_MAX)) {
        KERNEL_LOG(__func__, "Syscall number out of bounds");
        return (uint64_t)-ENOSYS;
    }
    
    syscall_handler_t handler = syscall_table[num];
    if (UNLIKELY(!handler)) {
        KERNEL_LOG(__func__, "Unimplemented syscall");
        return (uint64_t)-ENOSYS;
    }
    
    return handler(args);
}

//==================================================================
// SYSCALL IMPLEMENTATIONS
//==================================================================

// Helper macro to reduce boilerplate in syscall implementations
#define SYSCALL_ASM_TEMPLATE(name, constraints_in, constraints_out, clobbers) \
    register int64_t ret __asm__("rax");                                      \
    __asm__ volatile (                                                        \
        "syscall"                                                             \
        : constraints_out                                                     \
        : constraints_in                                                      \
        : clobbers                                                           \
    );                                                                       \
    return (uint64_t)ret;

SYSCALL_INLINE uint64_t sys_write(const syscall_args_t *a) {
    int fd           = (int)a->arg0;
    const void* buf  = (const void*)a->arg1;
    size_t count     = (size_t)a->arg2;
    
    SYSCALL_ASM_TEMPLATE(
        SYS_WRITE,
        "a"(SYS_WRITE), "D"(fd), "S"(buf), "d"(count),
        "=a"(ret),
        "rcx", "r11", "memory"
    )
}

SYSCALL_INLINE uint64_t sys_read(const syscall_args_t *a) {
    int fd       = (int)a->arg0;
    void* buf    = (void*)a->arg1;
    size_t count = (size_t)a->arg2;
    
    SYSCALL_ASM_TEMPLATE(
        SYS_READ,
        "a"(SYS_READ), "D"(fd), "S"(buf), "d"(count),
        "=a"(ret),
        "rcx", "r11", "memory"
    )
}

SYSCALL_INLINE uint64_t sys_open(const syscall_args_t *a) {
    const char *path = (const char*)a->arg0;
    int flags        = (int)a->arg1;
    mode_t mode      = (mode_t)a->arg2;
    
    SYSCALL_ASM_TEMPLATE(
        SYS_OPEN,
        "a"(SYS_OPEN), "D"(path), "S"(flags), "d"(mode),
        "=a"(ret),
        "rcx", "r11", "memory"
    )
}

SYSCALL_INLINE uint64_t sys_close(const syscall_args_t *a) {
    int fd = (int)a->arg0;
    
    SYSCALL_ASM_TEMPLATE(
        SYS_CLOSE,
        "a"(SYS_CLOSE), "D"(fd),
        "=a"(ret),
        "rcx", "r11"
    )
}

SYSCALL_INLINE uint64_t sys_lseek(const syscall_args_t *a) {
    int fd       = (int)a->arg0;
    off_t offset = (off_t)a->arg1;
    int whence   = (int)a->arg2;
    
    SYSCALL_ASM_TEMPLATE(
        SYS_LSEEK,
        "a"(SYS_LSEEK), "D"(fd), "S"(offset), "d"(whence),
        "=a"(ret),
        "rcx", "r11"
    )
}

SYSCALL_INLINE uint64_t sys_fstat(const syscall_args_t *a) {
    int fd          = (int)a->arg0;
    struct stat *st = (struct stat*)a->arg1;
    
    SYSCALL_ASM_TEMPLATE(
        SYS_FSTAT,
        "a"(SYS_FSTAT), "D"(fd), "S"(st),
        "=a"(ret),
        "rcx", "r11", "memory"
    )
}

SYSCALL_INLINE uint64_t sys_exit(const syscall_args_t *a) {
    int status = (int)a->arg0;
    
    __asm__ volatile (
        "syscall"
        : /* no outputs - this call never returns */
        : "a"(SYS_EXIT), "D"(status)
        : "rcx", "r11", "memory"
    );
    
    __builtin_unreachable();
}

#undef SYSCALL_ASM_TEMPLATE

//==================================================================
// OPTIONAL: SYSCALL STATISTICS (for debugging/profiling)
//==================================================================

#ifdef SYSCALL_STATS
static struct {
    uint64_t count;
    uint64_t errors;
} syscall_stats[SYS_MAX] CACHE_ALIGNED;

uint64_t syscall_entry_with_stats(uint64_t num, const syscall_args_t *args) {
    uint64_t result = syscall_entry(num, args);
    
    if (LIKELY(num < SYS_MAX)) {
        __atomic_fetch_add(&syscall_stats[num].count, 1, __ATOMIC_RELAXED);
        if (UNLIKELY((int64_t)result < 0)) {
            __atomic_fetch_add(&syscall_stats[num].errors, 1, __ATOMIC_RELAXED);
        }
    }
    
    return result;
}
#endif
