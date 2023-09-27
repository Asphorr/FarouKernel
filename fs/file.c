#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#define MAX_NAME_LEN 256
#define MAX_FILE_SIZE 1024*1024

struct File {
    char name[MAX_NAME_LEN];
    int fd;
    off_t size;
};

static int open(const char *filename, int flags) {
    int fd = -1;
    if ((fd = open(filename, flags)) == -1) {
        perror("open");
        return -1;
    }
    return fd;
}

static void close(int fd) {
    if (close(fd) == -1) {
        perror("close");
    }
}

static ssize_t read(int fd, void *buf, size_t count) {
    ssize_t ret = -1;
    while (ret == -1 || ret == 0) {
        ret = read(fd, buf, count);
        if (ret == -1) {
            perror("read");
        }
    }
    return ret;
}

static ssize_t write(int fd, const void *buf, size_t count) {
    ssize_t ret = -1;
    while (ret == -1 || ret == 0) {
        ret = write(fd, buf, count);
        if (ret == -1) {
            perror("write");
        }
    }
    return ret;
}

static off_t lseek(int fd, off_t offset, int whence) {
    off_t ret = -1;
    while (ret == -1) {
        ret = lseek(fd, offset, whence);
        if (ret == -1) {
            perror("lseek");
        }
    }
    return ret;
}

static int stat(const char *filename, struct stat *st) {
    if (stat(filename, st) == -1) {
        perror("stat");
        return -1;
    }
    return 0;
}

static int fstat(int fd, struct stat *st) {
    if (fstat(fd, st) == -1) {
        perror("fstat");
        return -1;
    }
    return 0;
}

int main() {
    char filename[] = "/home/user/example.txt";
    struct File *file = malloc(sizeof(struct File));
    file->name = filename;
    file->fd = open(filename, O_RDONLY);
    file->size = lseek(file->fd, 0, SEEK_END);

    printf("File name: %s\n", file->name);
    printf("File size: %ld\n", file->size);

    char buff[1024];
    ssize_t ret = read(file->fd, buff, 1024);
    if (ret == -1) {
        perror("read");
    } else {
        printf("Read %ld bytes\n", ret);
    }

    ret = write(file->fd, buff, 1024);
    if (ret == -1) {
        perror("write");
    } else {
        printf("Wrote %ld bytes\n", ret);
    }

    ret = lseek(file->fd, 0, SEEK_SET);
    if (ret == -1) {
        perror("lseek");
    } else {
        printf("File pos: %ld\n", ret);
    }

    free(file);
    return 0;
}
