#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h> // For bool type
#include <limits.h>  // For LONG_MAX, LONG_MIN

#define DEFAULT_BUFFER_SIZE 1024
#define MAX_RETRIES 3
#define LOG_FILE "error.log"

// Use const for members that the thread function won't modify
typedef struct {
    const char *filename;
    const char *data;
    size_t      data_len; // Store length to avoid repeated strlen
    size_t      buffer_size;
} file_task_t;

// --- Error Handling ---

// Improved logging: Fallback to stderr if log file fails
void log_error(const char *msg) {
    // Use 'errno' immediately before any other call might change it
    int saved_errno = errno;
    FILE *log_file = fopen(LOG_FILE, "a");
    if (log_file) {
        fprintf(log_file, "Error: %s - %s\n", msg, strerror(saved_errno));
        fclose(log_file);
    } else {
        // Fallback to stderr if log file cannot be opened
        fprintf(stderr, "Log Error: Could not open log file '%s'.\n", LOG_FILE);
        fprintf(stderr, "Original Error: %s - %s\n", msg, strerror(saved_errno));
    }
    // Print to stderr as well
    errno = saved_errno; // Restore errno for perror
    perror(msg);
}

// Consistent fatal error handler
void handle_fatal_error(const char *msg) {
    log_error(msg);
    exit(EXIT_FAILURE);
}

// --- Worker Thread ---

void *process_file(void *arg) {
    file_task_t *task = (file_task_t *)arg;
    int fd = -1; // Initialize to -1 to indicate not opened
    char *buffer = NULL;
    ssize_t bytes_written_total = 0;
    ssize_t bytes_op = 0; // Bytes read/written in a single operation
    int retries;

    // Allocate buffer (+1 for potential null terminator if needed, though not strictly necessary with current read loop)
    // Using calloc to zero-initialize (good practice, though read will overwrite)
    buffer = calloc(1, task->buffer_size);
    if (buffer == NULL) {
        // Cannot use log_error safely if logging needs malloc/fopen, print directly
        perror("Memory allocation failed for buffer");
        // We can't call handle_fatal_error as it might try to log
        exit(EXIT_FAILURE); // Or return an error status if threads should continue
    }

    // Open the file for reading and writing, create if doesn't exist, truncate if exists
    // Permissions: User read/write
    fd = open(task->filename, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        log_error("Error opening file");
        goto cleanup; // Use goto for centralized cleanup
    }

    // --- Write data to the file (handle partial writes) ---
    printf("Thread %lu: Writing %zu bytes to %s\n", (unsigned long)pthread_self(), task->data_len, task->filename);
    retries = 0;
    while (bytes_written_total < task->data_len) {
        bytes_op = write(fd, task->data + bytes_written_total, task->data_len - bytes_written_total);

        if (bytes_op == -1) {
            // Error during write
            if (errno == EINTR) { // Interrupted by signal, just retry
                 printf("Thread %lu: Write interrupted, retrying...\n", (unsigned long)pthread_self());
                 continue;
            }
            if (retries < MAX_RETRIES) {
                retries++;
                 printf("Thread %lu: Write error, retry %d/%d...\n", (unsigned long)pthread_self(), retries, MAX_RETRIES);
                 log_error("Retry writing to file");
                 // Optional: Add a small delay before retrying
                 // usleep(10000); // e.g., 10ms
            } else {
                log_error("Error writing to file after retries");
                goto cleanup;
            }
        } else if (bytes_op == 0) {
             // Should not happen with regular files unless disk full?
             fprintf(stderr, "Thread %lu: Wrote 0 bytes, potential issue (disk full?).\n", (unsigned long)pthread_self());
             log_error("Wrote 0 bytes unexpectedly");
             goto cleanup;
        }
        else {
            // Successful write (partial or full)
            bytes_written_total += bytes_op;
            retries = 0; // Reset retries on successful write
        }
    }
     printf("Thread %lu: Finished writing to %s\n", (unsigned long)pthread_self(), task->filename);


    // Reset the file offset to the beginning for reading
    if (lseek(fd, 0, SEEK_SET) == -1) {
        log_error("Error seeking in file");
        goto cleanup;
    }

    // --- Read data from the file ---
    printf("Thread %lu: Reading content from %s:\n", (unsigned long)pthread_self(), task->filename);
    retries = 0;
    printf("--- Start content of %s ---\n", task->filename);
    while (true) {
        bytes_op = read(fd, buffer, task->buffer_size);

        if (bytes_op == -1) {
            // Error during read
             if (errno == EINTR) { // Interrupted by signal, just retry
                 printf("Thread %lu: Read interrupted, retrying...\n", (unsigned long)pthread_self());
                 continue;
            }
            if (retries < MAX_RETRIES) {
                retries++;
                 printf("Thread %lu: Read error, retry %d/%d...\n", (unsigned long)pthread_self(), retries, MAX_RETRIES);
                 log_error("Retry reading from file");
                // Optional: Add a small delay before retrying
                // usleep(10000);
            } else {
                log_error("Error reading from file after retries");
                goto cleanup;
            }
        } else if (bytes_op == 0) {
            // End Of File (EOF)
            break;
        } else {
            // Successfully read 'bytes_op' bytes
            // Print the chunk read (use write for thread-safety with stdout if needed,
            // but printf is often okay for simple examples if output mingling is acceptable)
            // Using fwrite for potentially better handling of binary data / nulls
             fwrite(buffer, 1, bytes_op, stdout);
             retries = 0; // Reset retries on successful read
        }
    }
    printf("\n--- End content of %s ---\n", task->filename);


// Cleanup label: Ensures resources are released
cleanup:
    // Close file if it was opened
    if (fd != -1) {
        if (close(fd) == -1) {
            // Log error, but continue cleanup
            log_error("Error closing file");
        }
    }
    // Free buffer if it was allocated
    free(buffer); // free(NULL) is safe

    // In this design, an error in a thread doesn't stop others,
    // but we could return a status if needed.
    return NULL;
}

// --- Main Function ---

void print_usage(const char *prog_name) {
     fprintf(stderr, "Usage: %s [-b buffer_size] <file1> <data1> [<file2> <data2> ...]\n", prog_name);
     fprintf(stderr, "  -b buffer_size : Optional buffer size (default: %d)\n", DEFAULT_BUFFER_SIZE);
}

int main(int argc, char *argv[]) {
    size_t buffer_size = DEFAULT_BUFFER_SIZE;
    int opt;
    long conv_buffer_size;
    char *endptr;

    // --- Argument Parsing using getopt ---
    // Note: getopt might reorder argv, handle file/data pairs after loop
    while ((opt = getopt(argc, argv, "b:")) != -1) {
        switch (opt) {
            case 'b':
                errno = 0; // Reset errno before call
                conv_buffer_size = strtol(optarg, &endptr, 10);

                // Error checking for strtol
                if (optarg == endptr || *endptr != '\0') {
                    fprintf(stderr, "Error: Invalid buffer size value: '%s'. Not a valid number.\n", optarg);
                    print_usage(argv[0]);
                    exit(EXIT_FAILURE);
                }
                if ((errno == ERANGE && (conv_buffer_size == LONG_MAX || conv_buffer_size == LONG_MIN)) || (errno != 0 && conv_buffer_size == 0)) {
                    fprintf(stderr, "Error: Buffer size value out of range: '%s'.\n", optarg);
                    print_usage(argv[0]);
                    exit(EXIT_FAILURE);
                }
                 if (conv_buffer_size <= 0) {
                     fprintf(stderr, "Error: Buffer size must be positive: '%s'.\n", optarg);
                     print_usage(argv[0]);
                     exit(EXIT_FAILURE);
                 }
                 buffer_size = (size_t)conv_buffer_size;
                 printf("Using buffer size: %zu\n", buffer_size);
                break;
            case '?':
                // getopt prints an error message, or handle unknown option
                 fprintf(stderr, "Error: Unknown option '-%c'.\n", optopt);
                 print_usage(argv[0]);
                exit(EXIT_FAILURE);
            default:
                // Should not happen
                abort();
        }
    }

    // --- Process File/Data Pairs ---
    // 'optind' is the index of the first non-option argument
    int remaining_args = argc - optind;

    if (remaining_args < 2 || remaining_args % 2 != 0) {
        fprintf(stderr, "Error: Invalid number of file/data pairs provided.\n");
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    int pairs = remaining_args / 2;
    pthread_t *threads = NULL;
    file_task_t *tasks = NULL;

    // Allocate memory for threads and tasks arrays
    threads = malloc(pairs * sizeof(pthread_t));
    tasks = malloc(pairs * sizeof(file_task_t));

    if (threads == NULL || tasks == NULL) {
        perror("Failed to allocate memory for threads/tasks");
        // Need to free whichever one succeeded, if any
        free(threads);
        free(tasks);
        exit(EXIT_FAILURE);
    }


    printf("Starting %d file processing tasks...\n", pairs);

    // --- Create Threads ---
    for (int i = 0; i < pairs; i++) {
        tasks[i].filename = argv[optind + 2 * i];     // First non-option is file1
        tasks[i].data = argv[optind + 2 * i + 1]; // Second non-option is data1
        tasks[i].buffer_size = buffer_size;
        tasks[i].data_len = strlen(tasks[i].data); // Calculate length once

        printf("  Task %d: file='%s', data_len=%zu\n", i, tasks[i].filename, tasks[i].data_len);

        int create_status = pthread_create(&threads[i], NULL, process_file, &tasks[i]);
        if (create_status != 0) {
            // Error creating thread - log, but maybe try to continue?
            // For a fatal approach:
            errno = create_status; // Set errno for perror based on pthread_create's return
            perror("Error creating thread");
            // We should ideally wait for already created threads, but for simplicity:
            fprintf(stderr, "Fatal: Could not create thread %d. Exiting.\n", i);
            // Cleanup allocated memory before exiting
            free(threads);
            free(tasks);
            exit(EXIT_FAILURE);
        }
    }

    // --- Wait for Threads to Complete ---
    printf("Waiting for tasks to complete...\n");
    bool all_joined_ok = true;
    for (int i = 0; i < pairs; i++) {
        int join_status = pthread_join(threads[i], NULL);
        if (join_status != 0) {
            errno = join_status;
            perror("Error joining thread");
            fprintf(stderr, "Warning: Failed to join thread for task %d (file: %s)\n", i, tasks[i].filename);
            all_joined_ok = false;
            // Continue trying to join other threads
        }
    }

    // --- Cleanup ---
    free(threads);
    free(tasks);

    if (all_joined_ok) {
        printf("All tasks completed successfully.\n");
        return 0;
    } else {
        printf("Some tasks may not have completed successfully (join failed).\n");
        return 1; // Indicate potential issue
    }
}
