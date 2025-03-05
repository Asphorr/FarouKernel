#ifndef SYSCALLS_H
#define SYSCALLS_H

#include <stdint.h>
#include <sys/stat.h>

// System call numbers
enum syscall_number {
    SYS_WRITE = 1,
    SYS_READ,
    SYS_OPEN,
    SYS_CLOSE,
    SYS_LSEEK,
    SYS_FSTAT,
    SYS_EXIT = 60,
    SYS_MAX
};

// Unified handler type for all system calls
typedef uint64_t (*syscall_handler_t)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);

// System call handlers with unified signature
uint64_t sys_write(uint64_t fd, uint64_t buf, uint64_t count, uint64_t unused1, uint64_t unused2);
uint64_t sys_read(uint64_t fd, uint64_t buf, uint64_t count, uint64_t unused1, uint64_t unused2);
uint64_t sys_open(uint64_t path, uint64_t flags, uint64_t mode, uint64_t unused1, uint64_t unused2);
uint64_t sys_close(uint64_t fd, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4);
uint64_t sys_lseek(uint64_t fd, uint64_t offset, uint64_t whence, uint64_t unused1, uint64_t unused2);
uint64_t sys_fstat(uint64_t fd, uint64_t st_ptr, uint64_t unused1, uint64_t unused2, uint64_t unused3);
uint64_t sys_exit(uint64_t status, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4) __attribute__((noreturn));

// System call entry point with correct return type
uint64_t syscall_entry(uint64_t syscall_num, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4);

// Error logging declaration remains
void log_error(const char *message);

#endif // SYSCALLS_H
