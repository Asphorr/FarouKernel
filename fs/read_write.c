#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>

#define DEFAULT_BUFFER_SIZE 1024
#define MAX_RETRIES 3

typedef struct {
    char *filename;
    char *data;
    size_t buffer_size;
} file_task_t;

void log_error(const char *msg) {
    FILE *log_file = fopen("error.log", "a");
    if (log_file) {
        fprintf(log_file, "Error: %s - %s\n", msg, strerror(errno));
        fclose(log_file);
    }
    perror(msg);
}

void handle_error(const char *msg) {
    log_error(msg);
    exit(EXIT_FAILURE);
}

void *process_file(void *arg) {
    file_task_t *task = (file_task_t *)arg;
    int fd, retries = 0;
    ssize_t ret;
    char *buffer = (char *)malloc(task->buffer_size);
    if (buffer == NULL) {
        handle_error("Memory allocation failed");
    }

    // Open the file for reading and writing
    fd = open(task->filename, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        free(buffer);
        handle_error("Error opening file");
    }

    // Write data to the file
    while ((ret = write(fd, task->data, strlen(task->data))) == -1 && retries < MAX_RETRIES) {
        retries++;
        log_error("Retry writing to file");
    }
    if (ret == -1) {
        close(fd);
        free(buffer);
        handle_error("Error writing to file");
    }

    // Reset the file offset to the beginning
    if (lseek(fd, 0, SEEK_SET) == -1) {
        close(fd);
        free(buffer);
        handle_error("Error seeking in file");
    }

    // Read data from the file
    retries = 0;
    while ((ret = read(fd, buffer, task->buffer_size - 1)) == -1 && retries < MAX_RETRIES) {
        retries++;
        log_error("Retry reading from file");
    }
    if (ret == -1) {
        close(fd);
        free(buffer);
        handle_error("Error reading from file");
    }

    // Null-terminate the buffer for safe printing
    buffer[ret] = '\0';

    // Print the contents of the file
    printf("Read content from %s:\n%s", task->filename, buffer);

    // Close the file
    if (close(fd) == -1) {
        free(buffer);
        handle_error("Error closing file");
    }

    free(buffer);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <file1> <data1> [<file2> <data2> ...] [-b buffer_size]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    size_t buffer_size = DEFAULT_BUFFER_SIZE;
    int pairs = (argc - 1) / 2;

    // Check for buffer size argument
    if ((argc % 2 == 0) && strcmp(argv[argc - 2], "-b") == 0) {
        buffer_size = atoi(argv[argc - 1]);
        pairs--;
    }

    pthread_t threads[pairs];
    file_task_t tasks[pairs];

    for (int i = 0; i < pairs; i++) {
        tasks[i].filename = argv[2 * i + 1];
        tasks[i].data = argv[2 * i + 2];
        tasks[i].buffer_size = buffer_size;

        if (pthread_create(&threads[i], NULL, process_file, &tasks[i]) != 0) {
            handle_error("Error creating thread");
        }
    }

    for (int i = 0; i for (int i = 0; i < pairs; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            handle_error("Error joining thread");
        }
    }

    printf("All tasks completed successfully.\n");
    return 0;
}
