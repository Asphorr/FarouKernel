#ifndef SYSCALLS_H
#define SYSCALLS_H

#include <stdint.h>
#include <sys/stat.h>

#define KERNEL_ALIGN         __attribute__((aligned(64)))
#define FORCE_INLINE         __attribute__((always_inline, flatten)) static inline
#define NO_RETURN            __attribute__((noreturn))
#define LIKELY(x)            __builtin_expect(!!(x), 1)
#define UNLIKELY(x)          __builtin_expect(!!(x), 0)
#define SYSCALL_HANDLER      FORCE_INLINE uint64_t

// System call numbers with cache-line alignment
enum syscall_numbers {
    SYS_WRITE = 1,
    SYS_READ,
    SYS_OPEN,
    SYS_CLOSE,
    SYS_LSEEK,
    SYS_FSTAT,
    SYS_EXIT = 60,
    SYS_MAX
} KERNEL_ALIGN;

// Compact argument structure with register packing
typedef struct syscall_args {
    union {
        struct { uint64_t a, b, c, d; };
        uint64_t args[4];
    };
} syscall_args_t KERNEL_ALIGN;

// Unified handler type with register optimization
typedef uint64_t (*syscall_handler_t)(const syscall_args_t*) KERNEL_ALIGN;

// Assembly optimized entry point
uint64_t syscall_entry(uint64_t num, const syscall_args_t *args) 
    __attribute__((hot, regparm(2)));

// System call handlers with register-based parameters
SYSCALL_HANDLER sys_write(const syscall_args_t *args);
SYSCALL_HANDLER sys_read(const syscall_args_t *args);
SYSCALL_HANDLER sys_open(const syscall_args_t *args);
SYSCALL_HANDLER sys_close(const syscall_args_t *args);
SYSCALL_HANDLER sys_lseek(const syscall_args_t *args);
SYSCALL_HANDLER sys_fstat(const syscall_args_t *args);
SYSCALL_HANDLER sys_exit(const syscall_args_t *args) NO_RETURN;

// Lock-free error logging macro
#define KERNEL_LOG(msg) \
    __asm__ volatile("movq %0, %%rdi; int $0x80" :: "r"(msg) : "rdi", "memory")

#endif // SYSCALLS_H
