#pragma once

#include <stddef.h>
#include <stdio.h>
#include <string.h>

// System call numbers
enum {
    SYSCALL_EXIT,
    SYSCALL_READ,
    SYSCALL_WRITE,
    SYSCALL_OPEN,
    SYSCALL_CLOSE,
    SYSCALL_CREATE,
    SYSCALL_DELETE,
    SYSCALL_GETPID,
    SYSCALL_SLEEP,
};

// Structure for storing system call information
struct syscall {
    enum {
        EXIT,
        READ,
        WRITE,
        OPEN,
        CLOSE,
        CREATE,
        DELETE,
        GETPID,
        SLEEP,
    };
    union {
        int number;       // System call number
        void (*function)(void); // Function to execute for the system call
    };
};

// Define external variables and functions
extern int syscall(struct syscall *sc);
extern int exit(int status);
extern ssize_t read(int fd, void *buf, size_t count);
extern ssize_t write(int fd, const void *buf, size_t count);
extern int open(const char *pathname, int flags);
extern int close(int fd);
extern int creat(const char *pathname, int mode);
extern int unlink(const char *pathname);
extern pid_t getpid(void);
extern unsigned int sleep(unsigned int seconds);

#endif /* SYSCall_H */
