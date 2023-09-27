#ifndef SYSCall_H
#define SYSCall_H

// System call numbers
#define SYSCALL_NUMBER_exit 1
#define SYSCALL_NUMBER_read 2
#define SYSCALL_NUMBER_write 3
#define SYSCALL_NUMBER_open 4
#define SYSCALL_NUMBER_close 5
#define SYSCALL_NUMBER_create 6
#define SYSCALL_NUMBER_delete 7
#define SYSCALL_NUMBER_getpid 8
#define SYSCALL_NUMBER_sleep 9

// Structure for storing system call information
typedef struct {
    int number;   // System call number
    void (*function)(void); // Function to execute for the system call
} syscall_t;

// Declare external variables and functions
extern int syscall(syscall_t *sc);
extern int exit(int status);
extern int read(int fd, char *buf, int len);
extern int write(int fd, const char *buf, int len);
extern int open(const char *filename, int flags);
extern int close(int fd);
extern int create(const char *filename, int mode);
extern int delete(const char *filename);
extern int getpid();
extern int sleep(int seconds);

#endif // SYSCall_H
