#ifndef _FILE_H
#define _FILE_H

// Structure to represent a file
struct File {
    char *name; // Name of the file
    int fd;     // File descriptor
    off_t size; // Size of the file in bytes
};

// Function prototypes
int open(const char *filename, int flags);
void close(int fd);
ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);
off_t lseek(int fd, off_t offset, int whence);
int stat(const char *filename, struct stat *st);
int fstat(int fd, struct stat *st);

#endif /* _FILE_H */
