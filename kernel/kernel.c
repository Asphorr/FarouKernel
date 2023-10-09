// Kernel.c

#include "Kernel.h"

static file_t files[MAX_FILES];

void init(file_t *files) {
    memset(files, 0, sizeof(files));
}

int openFile(const char *pathname, int flags) {
    int i = 0;
    while (i < MAX_FILES && strcmp(files[i].path, pathname)) {
        i++;
    }
    if (i == MAX_FILES) {
        return -ENOENT;
    } else {
        files[i].fd = open(pathname, flags);
        if (files[i].fd >= 0) {
            return files[i].fd;
        } else {
            return -EIO;
        }
    }
}

ssize_t readFile(int fd, void *buf, size_t count) {
    ssize_t retval = read(fd, buf, count);
    if (retval > 0) {
        return retval;
    } else if (retval == 0) {
        return -EOF;
    } else {
        return -EIO;
    }
}

ssize_t writeFile(int fd, const void *buf, size_t count) {
    ssize_t retval = write(fd, buf, count);
    if (retval > 0) {
        return retval;
    } else if (retval == 0) {
        return -EOF;
    } else {
        return -EIO;
    }
}

off_t lseekFile(int fd, off_t offset, int whence) {
    off_t retval = lseek(fd, offset, whence);
    if (retval != (off_t)-1) {
        return retval;
    } else {
        return -EINVAL;
    }
}

int closeFile(int fd) {
    int retval = close(fd);
    if (retval == 0) {
        return 0;
    } else {
        return -EIO;
    }
}

// Switch to the new thread
switch_to(new_thread);
