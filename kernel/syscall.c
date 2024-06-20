#include "syscalls.h"
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>

// Simple error logging function
void log_error(const char *message) {
    fprintf(stderr, "Error: %s (errno: %d)\n", message, errno);
}

// System call table
typedef size_t (*syscall_handler_t)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);
static const syscall_handler_t syscall_table[SYS_MAX] = {
    [SYS_WRITE] = (syscall_handler_t)sys_write,
    [SYS_READ] = (syscall_handler_t)sys_read,
    [SYS_OPEN] = (syscall_handler_t)sys_open,
    [SYS_CLOSE] = (syscall_handler_t)sys_close,
    [SYS_LSEEK] = (syscall_handler_t)sys_lseek,
    [SYS_FSTAT] = (syscall_handler_t)sys_fstat,
    [SYS_EXIT] = (syscall_handler_t)sys_exit,
};

// System call entry point
size_t syscall_entry(uint64_t syscall_num, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4) {
    if (syscall_num >= SYS_MAX || syscall_table[syscall_num] == NULL) {
        log_error("Invalid system call number");
        errno = ENOSYS;
        return (size_t)-1;
    }

    return syscall_table[syscall_num](arg1, arg2, arg3, arg4, 0);
}

// System call handlers implementation
size_t sys_write(int fd, const void *buf, size_t count) {
    ssize_t result = write(fd, buf, count);
    if (result < 0) {
        log_error("sys_write failed");
        return (size_t)-1;
    }
    return (size_t)result;
}

size_t sys_read(int fd, void *buf, size_t count) {
    ssize_t result = read(fd, buf, count);
    if (result < 0) {
        log_error("sys_read failed");
        return (size_t)-1;
    }
    return (size_t)result;
}

int sys_open(const char *path, int flags, int mode) {
    int fd = open(path, flags, mode);
    if (fd < 0) {
        log_error("sys_open failed");
        return -1;
    }
    return fd;
}

int sys_close(int fd) {
    int result = close(fd);
    if (result < 0) {
        log_error("sys_close failed");
        return -1;
    }
    return 0;
}

off_t sys_lseek(int fd, off_t offset, int whence) {
    off_t result = lseek(fd, offset, whence);
    if (result == (off_t)-1) {
        log_error("sys_lseek failed");
        return (off_t)-1;
    }
    return result;
}

int sys_fstat(int fd, struct stat *st) {
    int result = fstat(fd, st);
    if (result < 0) {
        log_error("sys_fstat failed");
        return -1;
    }
    return 0;
}

void sys_exit(int status) {
    _exit(status);
}

// Main function for testing
int main(void) {
    // Test sys_open and sys_write
    int fd = sys_open("testfile.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd != -1) {
        const char *message = "Hello, world!";
        size_t bytes_written = sys_write(fd, message, 13);
        printf("Bytes written: %zu\n", bytes_written);
        sys_close(fd);
    } else {
        log_error("Failed to open file for writing");
        return 1;
    }

    // Test sys_read
    fd = sys_open("testfile.txt", O_RDONLY, 0);
    if (fd != -1) {
        char buffer[20] = {0};
        size_t bytes_read = sys_read(fd, buffer, sizeof(buffer) - 1);
        if (bytes_read != (size_t)-1) {
            printf("Read from file: %s\n", buffer);
        } else {
            log_error("Failed to read from file");
        }
        sys_close(fd);
    } else {
        log_error("Failed to open file for reading");
        return 1;
    }

    // Test sys_lseek and sys_read
    fd = sys_open("testfile.txt", O_RDONLY, 0);
    if (fd != -1) {
        off_t new_offset = sys_lseek(fd, 7, SEEK_SET);
        if (new_offset != (off_t)-1) {
            char buffer[20] = {0};
            size_t bytes_read = sys_read(fd, buffer, sizeof(buffer) - 1);
            if (bytes_read != (size_t)-1) {
                printf("Read from offset %lld: %s\n", (long long)new_offset, buffer);
            } else {
                log_error("Failed to read from file after seek");
            }
        } else {
            log_error("Failed to seek in file");
        }
        sys_close(fd);
    } else {
        log_error("Failed to open file for seeking");
        return 1;
    }

    // Test sys_fstat
    fd = sys_open("testfile.txt", O_RDONLY, 0);
    if (fd != -1) {
        struct stat st;
        if (sys_fstat(fd, &st) == 0) {
            printf("File size: %lld bytes\n", (long long)st.st_size);
            printf("File mode: %o\n", st.st_mode & 0777);
        } else {
            log_error("Failed to get file stats");
        }
        sys_close(fd);
    } else {
        log_error("Failed to open file for stat");
        return 1;
    }

    // Test sys_exit
    printf("Exiting with code 0\n");
    sys_exit(0);

    // This line should never be reached
    return 1;
}
