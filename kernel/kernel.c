#include "Kernel.h"

static file_t files[MAX_FILES];

void init() {
    // Initialize the file system
    fs_init();

    // Set up the interrupt handlers
    setup_interrupts();

    // Start the first thread
    start_first_thread();
}

int openFile(const char *pathname, int flags) {
    // Check if the file exists
    if (!fs_exists(pathname)) {
        return -ENOENT;
    }

    // Open the file
    int fd = fs_open(pathname, flags);
    if (fd < 0) {
        return -EIO;
    }

    // Return the file descriptor
    return fd;
}

ssize_t readFile(int fd, void *buf, size_t count) {
    // Read data from the file
    ssize_t bytesRead = fs_read(fd, buf, count);
    if (bytesRead <= 0) {
        return -EIO;
    }

    // Return the number of bytes read
    return bytesRead;
}

ssize_t writeFile(int fd, const void *buf, size_t count) {
    // Write data to the file
    ssize_t bytesWritten = fs_write(fd, buf, count);
    if (bytesWritten <= 0) {
        return -EIO;
    }

    // Return the number of bytes written
    return bytesWritten;
}

off_t lseekFile(int fd, off_t offset, int whence) {
    // Seek to the specified position in the file
    off_t pos = fs_lseek(fd, offset, whence);
    if (pos < 0) {
        return -EINVAL;
    }

    // Return the current position in the file
    return pos;
}

int closeFile(int fd) {
    // Close the file
    int result = fs_close(fd);
    if (result < 0) {
        return -EIO;
    }

    // Return success
    return 0;
}

// Switch to the new thread
switch_to(new_thread);
