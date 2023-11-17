#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#define BUFFER_SIZE 1024

void handle_error(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    int fd;
    ssize_t ret;
    char buffer[BUFFER_SIZE];

    // Open the file
    fd = open("test.txt", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        handle_error("open");
    }

    // Write to the file
    ret = write(fd, "Hello, World!\n", 14);
    if (ret == -1) {
        handle_error("write");
    }

    // Reset the file offset to the beginning
    if (lseek(fd, 0, SEEK_SET) == -1) {
        handle_error("lseek");
    }

    // Read from the file in a loop until the end of the file is reached
    while ((ret = read(fd, buffer, BUFFER_SIZE)) > 0) {
        // Print the contents of the buffer
        printf("%.*s", (int)ret, buffer);
    }

    if (ret == -1) {
        handle_error("read");
    }

    // Reset the file offset to the beginning
    if (lseek(fd, 0, SEEK_SET) == -1) {
        handle_error("lseek");
    }

    // Write to the file in a loop until the end of the file is reached
    while ((ret = write(fd, buffer, BUFFER_SIZE)) > 0) {
        // Print the number of bytes written
        printf("Wrote %ld bytes\n", ret);
    }

    if (ret == -1) {
        handle_error("write");
    }

    // Close the file
    if (close(fd) == -1) {
        handle_error("close");
    }

    return 0;
}
