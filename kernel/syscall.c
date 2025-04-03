#include "syscalls.h"
#include <stdint.h>
#include <inttypes.h>

#define KERNEL_LOG(fn, msg) \
    __asm__ volatile("movq %0, %%rdi; movq %1, %%rsi; int $0x80" :: "r"(fn), "r"(msg) : "rdi", "rsi")

#define SYSCALL_INLINE __attribute__((always_inline, flatten)) static inline
#define LIKELY(x)      __builtin_expect(!!(x), 1)
#define UNLIKELY(x)    __builtin_expect(!!(x), 0)

typedef struct {
    union {
        struct { uint64_t a, b, c, d; };
        uint64_t args[4];
    };
} syscall_args_t;

typedef uint64_t (*syscall_handler_t)(const syscall_args_t*);
static const syscall_handler_t syscall_table[SYS_MAX] __attribute__((aligned(64))) = {
    [SYS_WRITE] = sys_write,
    [SYS_READ]  = sys_read,
    [SYS_OPEN]  = sys_open,
    [SYS_CLOSE] = sys_close,
    [SYS_LSEEK] = sys_lseek,
    [SYS_FSTAT] = sys_fstat,
    [SYS_EXIT]  = sys_exit
};

uint64_t syscall_entry(uint64_t num, const syscall_args_t *args) {
    if (UNLIKELY(num >= SYS_MAX || !syscall_table[num])) {
        KERNEL_LOG(__func__, "Invalid syscall");
        return -ENOSYS;
    }
    return syscall_table[num](args);
}

SYSCALL_INLINE uint64_t sys_write(const syscall_args_t *a) {
    int fd; const char *buf; size_t count;
    fd = a->a; buf = (const char*)a->b; count = a->c;

    ssize_t ret;
    __asm__ volatile (
        "mov %1, %%rax\n"
        "syscall\n"
        : "=a"(ret)
        : "i"(SYS_write), "D"(fd), "S"(buf), "d"(count)
        : "rcx", "r11", "memory"
    );
    return LIKELY(ret >= 0) ? ret : -errno;
}

SYSCALL_INLINE uint64_t sys_read(const syscall_args_t *a) {
    int fd; char *buf; size_t count;
    fd = a->a; buf = (char*)a->b; count = a->c;

    register ssize_t ret __asm__("rax");
    __asm__ volatile (
        "syscall\n"
        : "=a"(ret)
        : "a"(SYS_read), "D"(fd), "S"(buf), "d"(count)
        : "rcx", "r11", "memory"
    );
    return LIKELY(ret >= 0) ? ret : -errno;
}

SYSCALL_INLINE uint64_t sys_open(const syscall_args_t *a) {
    const char *path = (const char*)a->a;
    int flags = a->b; mode_t mode = a->c;

    register int ret __asm__("rax");
    __asm__ volatile (
        "syscall\n"
        : "=a"(ret)
        : "a"(SYS_open), "D"(path), "S"(flags), "d"(mode)
        : "rcx", "r11", "memory"
    );
    return LIKELY(ret >= 0) ? ret : -errno;
}

SYSCALL_INLINE uint64_t sys_close(const syscall_args_t *a) {
    int fd = a->a;
    
    register int ret __asm__("rax");
    __asm__ volatile (
        "syscall\n"
        : "=a"(ret)
        : "a"(SYS_close), "D"(fd)
        : "rcx", "r11"
    );
    return LIKELY(ret == 0) ? 0 : -errno;
}

SYSCALL_INLINE uint64_t sys_lseek(const syscall_args_t *a) {
    int fd = a->a; off_t off = a->b; int whence = a->c;

    register off_t ret __asm__("rax");
    __asm__ volatile (
        "syscall\n"
        : "=a"(ret)
        : "a"(SYS_lseek), "D"(fd), "S"(off), "d"(whence)
        : "rcx", "r11"
    );
    return LIKELY(ret != (off_t)-1) ? ret : -errno;
}

SYSCALL_INLINE uint64_t sys_fstat(const syscall_args_t *a) {
    int fd = a->a; struct stat *st = (struct stat*)a->b;

    register int ret __asm__("rax");
    __asm__ volatile (
        "syscall\n"
        : "=a"(ret)
        : "a"(SYS_fstat), "D"(fd), "S"(st)
        : "rcx", "r11", "memory"
    );
    return LIKELY(ret == 0) ? 0 : -errno;
}

SYSCALL_INLINE uint64_t sys_exit(const syscall_args_t *a) {
    int status = a->a;
    __asm__ volatile (
        "mov %0, %%edi\n"
        "mov %1, %%rax\n"
        "syscall\n"
        :: "i"(SYS_exit), "i"(status)
        : "memory"
    );
    __builtin_unreachable();
}
