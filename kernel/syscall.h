#ifndef SYSCALLS_H
#define SYSCALLS_H

#include <stdint.h>
#include <stddef.h>
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

// System call handlers
size_t sys_write(int fd, const void *buf, size_t count);
size_t sys_read(int fd, void *buf, size_t count);
int sys_open(const char *path, int flags, int mode);
int sys_close(int fd);
off_t sys_lseek(int fd, off_t offset, int whence);
int sys_fstat(int fd, struct stat *st);
void sys_exit(int status) __attribute__((noreturn));

// System call entry point
size_t syscall_entry(uint64_t syscall_num, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4);

// Error logging function
void log_error(const char *message);

#endif // SYSCALLS_H
