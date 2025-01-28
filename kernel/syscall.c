#include "syscalls.h"
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdint.h>

// Error logging remains the same
void log_error(const char *message) {
    fprintf(stderr, "Error: %s (errno: %d)\n", message, errno);
}

// Fixed syscall handler type definition
typedef uint64_t (*syscall_handler_t)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);
static const syscall_handler_t syscall_table[SYS_MAX] = {
    [SYS_WRITE] = sys_write,
    [SYS_READ] = sys_read,
    [SYS_OPEN] = sys_open,
    [SYS_CLOSE] = sys_close,
    [SYS_LSEEK] = sys_lseek,
    [SYS_FSTAT] = sys_fstat,
    [SYS_EXIT] = sys_exit,
};

// Updated syscall entry point
uint64_t syscall_entry(uint64_t syscall_num, uint64_t arg1, uint64_t arg2, 
                      uint64_t arg3, uint64_t arg4) {
    if (syscall_num >= SYS_MAX || !syscall_table[syscall_num]) {
        log_error("Invalid system call number");
        errno = ENOSYS;
        return (uint64_t)-1;
    }
    return syscall_table[syscall_num](arg1, arg2, arg3, arg4, 0);
}

/* Updated handlers with proper signature */
uint64_t sys_write(uint64_t fd, uint64_t buf, uint64_t count, 
                  uint64_t unused1, uint64_t unused2) {
    ssize_t result = write((int)fd, (const void *)(uintptr_t)buf, (size_t)count);
    return (result < 0) ? (log_error("sys_write failed"), (uint64_t)-1) 
                       : (uint64_t)result;
}

uint64_t sys_read(uint64_t fd, uint64_t buf, uint64_t count,
                 uint64_t unused1, uint64_t unused2) {
    ssize_t result = read((int)fd, (void *)(uintptr_t)buf, (size_t)count);
    return (result < 0) ? (log_error("sys_read failed"), (uint64_t)-1)
                       : (uint64_t)result;
}

uint64_t sys_open(uint64_t path, uint64_t flags, uint64_t mode,
                 uint64_t unused1, uint64_t unused2) {
    int result = open((const char *)(uintptr_t)path, (int)flags, (mode_t)mode);
    return (result < 0) ? (log_error("sys_open failed"), (uint64_t)-1)
                       : (uint64_t)result;
}

uint64_t sys_close(uint64_t fd, uint64_t unused1, uint64_t unused2,
                  uint64_t unused3, uint64_t unused4) {
    int result = close((int)fd);
    return (result < 0) ? (log_error("sys_close failed"), (uint64_t)-1)
                       : (uint64_t)result;
}

uint64_t sys_lseek(uint64_t fd, uint64_t offset, uint64_t whence,
                  uint64_t unused1, uint64_t unused2) {
    off_t result = lseek((int)fd, (off_t)offset, (int)whence);
    return (result == (off_t)-1) ? (log_error("sys_lseek failed"), (uint64_t)-1)
                                : (uint64_t)result;
}

uint64_t sys_fstat(uint64_t fd, uint64_t st_ptr, uint64_t unused1,
                  uint64_t unused2, uint64_t unused3) {
    int result = fstat((int)fd, (struct stat *)(uintptr_t)st_ptr);
    return (result < 0) ? (log_error("sys_fstat failed"), (uint64_t)-1)
                       : (uint64_t)result;
}

uint64_t sys_exit(uint64_t status, uint64_t unused1, uint64_t unused2,
                 uint64_t unused3, uint64_t unused4) {
    _exit((int)status);
    return 0; // Never reached
}

// Test main remains largely the same with cast adjustments
int main(void) {
    // Test sys_open and sys_write
    uint64_t fd = sys_open((uint64_t)"testfile.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644, 0, 0);
    if (fd != (uint64_t)-1) {
        const char *message = "Hello, world!";
        uint64_t bytes_written = sys_write(fd, (uint64_t)message, 13, 0, 0);
        printf("Bytes written: %lu\n", (unsigned long)bytes_written);
        sys_close(fd, 0, 0, 0, 0);
    } else {
        log_error("Failed to open file for writing");
        return 1;
    }

    // Test sys_read
    fd = sys_open((uint64_t)"testfile.txt", O_RDONLY, 0, 0, 0);
    if (fd != (uint64_t)-1) {
        char buffer[20] = {0};
        uint64_t bytes_read = sys_read(fd, (uint64_t)buffer, sizeof(buffer)-1, 0, 0);
        if (bytes_read != (uint64_t)-1) {
            printf("Read from file: %s\n", buffer);
        } else {
            log_error("Failed to read from file");
        }
        sys_close(fd, 0, 0, 0, 0);
    } else {
        log_error("Failed to open file for reading");
        return 1;
    }

    // Test sys_lseek and sys_read
    fd = sys_open((uint64_t)"testfile.txt", O_RDONLY, 0, 0, 0);
    if (fd != (uint64_t)-1) {
        uint64_t new_offset = sys_lseek(fd, 7, SEEK_SET, 0, 0);
        if (new_offset != (uint64_t)-1) {
            char buffer[20] = {0};
            uint64_t bytes_read = sys_read(fd, (uint64_t)buffer, sizeof(buffer)-1, 0, 0);
            if (bytes_read != (uint64_t)-1) {
                printf("Read from offset %llu: %s\n", (unsigned long long)new_offset, buffer);
            } else {
                log_error("Failed to read from file after seek");
            }
        } else {
            log_error("Failed to seek in file");
        }
        sys_close(fd, 0, 0, 0, 0);
    } else {
        log_error("Failed to open file for seeking");
        return 1;
    }

    // Test sys_fstat
    fd = sys_open((uint64_t)"testfile.txt", O_RDONLY, 0, 0, 0);
    if (fd != (uint64_t)-1) {
        struct stat st;
        uint64_t result = sys_fstat(fd, (uint64_t)&st, 0, 0, 0);
        if (result != (uint64_t)-1) {
            printf("File size: %lld bytes\n", (long long)st.st_size);
            printf("File mode: %o\n", st.st_mode & 0777);
        } else {
            log_error("Failed to get file stats");
        }
        sys_close(fd, 0, 0, 0, 0);
    } else {
        log_error("Failed to open file for stat");
        return 1;
    }

    printf("Exiting with code 0\n");
    sys_exit(0, 0, 0, 0, 0);
    return 1;
}
