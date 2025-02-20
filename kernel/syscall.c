#include "syscalls.h"
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>

// Error logging function with function name
void log_error(const char *func_name, const char *message) {
    fprintf(stderr, "Error in %s: %s (errno: %d)\n", func_name, message, errno);
}

// System call handler type definition
typedef uint64_t (*syscall_handler_t)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);
static const syscall_handler_t syscall_table[SYS_MAX] = {
    [SYS_WRITE] = sys_write,
    [SYS_READ] = sys_read,
    [SYS_OPEN] = sys_open,
    [SYS_CLOSE] = sys_close,
    [SYS_LSEEK] = sys_lseek,
    [SYS_FSTAT] = sys_fstat,
    [SYS EXIT] = sys_exit,
};

// System call entry point
uint64_t syscall_entry(uint64_t syscall_num, uint64_t arg1, uint64_t arg2, 
                      uint64_t arg3, uint64_t arg4) {
    if (syscall_num >= SYS_MAX || !syscall_table[syscall_num]) {
        log_error("syscall_entry", "Invalid system call number");
        errno = ENOSYS;
        return (uint64_t)-1;
    }
    return syscall_table[syscall_num](arg1, arg2, arg3, arg4, 0);
}

// System call handlers with improved readability
uint64_t sys_write(uint64_t fd, uint64_t buf, uint64_t count, 
                  uint64_t unused1, uint64_t unused2) {
    int file_descriptor = (int)fd;
    const void *buffer = (const void *)(buf);
    size_t bytes_to_write = (size_t)count;
    ssize_t result = write(file_descriptor, buffer, bytes_to_write);
    return (result < 0) ? (log_error(__FUNCTION__, "write failed"), (uint64_t)-1) 
                       : (uint64_t)result;
}

uint64_t sys_read(uint64_t fd, uint64_t buf, uint64_t count,
                 uint64_t unused1, uint64_t unused2) {
    int file_descriptor = (int)fd;
    void *buffer = (void *)(buf);
    size_t bytes_to_read = (size_t)count;
    ssize_t result = read(file_descriptor, buffer, bytes_to_read);
    return (result < 0) ? (log_error(__FUNCTION__, "read failed"), (uint64_t)-1)
                       : (uint64_t)result;
}

uint64_t sys_open(uint64_t path, uint64_t flags, uint64_t mode,
                 uint64_t unused1, uint64_t unused2) {
    const char *file_path = (const char *)(path);
    int open_flags = (int)flags;
    mode_t file_mode = (mode_t)mode;
    int result = open(file_path, open_flags, file_mode);
    return (result < 0) ? (log_error(__FUNCTION__, "open failed"), (uint64_t)-1)
                       : (uint64_t)result;
}

uint64_t sys_close(uint64_t fd, uint64_t unused1, uint64_t unused2,
                  uint64_t unused3, uint64_t unused4) {
    int file_descriptor = (int)fd;
    int result = close(file_descriptor);
    return (result < 0) ? (log_error(__FUNCTION__, "close failed"), (uint64_t)-1)
                       : (uint64_t)result;
}

uint64_t sys_lseek(uint64_t fd, uint64_t offset, uint64_t whence,
                  uint64_t unused1, uint64_t unused2) {
    int file_descriptor = (int)fd;
    off_t seek_offset = (off_t)offset;
    int seek_whence = (int)whence;
    off_t result = lseek(file_descriptor, seek_offset, seek_whence);
    return (result == (off_t)-1) ? (log_error(__FUNCTION__, "lseek failed"), (uint64_t)-1)
                                : (uint64_t)result;
}

uint64_t sys_fstat(uint64_t fd, uint64_t st_ptr, uint64_t unused1,
                  uint64_t unused2, uint64_t unused3) {
    int file_descriptor = (int)fd;
    struct stat *stat_buffer = (struct stat *)(st_ptr);
    int result = fstat(file_descriptor, stat_buffer);
    return (result < 0) ? (log_error(__FUNCTION__, "fstat failed"), (uint64_t)-1)
                       : (uint64_t)result;
}

uint64_t sys_exit(uint64_t status, uint64_t unused1, uint64_t unused2,
                 uint64_t unused3, uint64_t unused4) {
    _exit((int)status);
    return 0; // Never reached
}

// Test main with improvements
int main(void) {
    // Test sys_open and sys_write
    const char *file_name = "testfile.txt";
    uint64_t fd = sys_open((uint64_t)file_name, O_WRONLY | O_CREAT | O_TRUNC, 0644, 0, 0);
    if (fd != (uint64_t)-1) {
        const char *message = "Hello, world!";
        size_t message_length = strlen(message);
        uint64_t bytes_written = sys_write(fd, (uint64_t)message, message_length, 0, 0);
        printf("Bytes written: %" PRIu64 "\n", bytes_written);
        sys_close(fd, 0, 0, 0, 0);
    } else {
        log_error("main", "Failed to open file for writing");
        return 1;
    }

    // Test sys_read
    fd = sys_open((uint64_t)file_name, O_RDONLY, 0, 0, 0);
    if (fd != (uint64_t)-1) {
        char buffer[20] = {0};
        uint64_t bytes_read = sys_read(fd, (uint64_t)buffer, sizeof(buffer)-1, 0, 0);
        if (bytes_read != (uint64_t)-1) {
            printf("Read from file: %s\n", buffer);
        } else {
            log_error("main", "Failed to read from file");
        }
        sys_close(fd, 0, 0, 0, 0);
    } else {
        log_error("main", "Failed to open file for reading");
        return 1;
    }

    // Test
