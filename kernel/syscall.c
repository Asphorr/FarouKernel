#include "syscall.h"
#include "memory.h"
#include "processes.h"
#include "driver.h"

// Define the system call table
static syscall_t syscall_table[SYSCALL_NUMBER_MAX];

// Initialize the system call table
void initialize_syscall_table() {
    syscall_table[SYSCALL_NUMBER_exit] = (syscall_t)exit;
    syscall_table[SYSCALL_NUMBER_read] = (syscall_t)read;
    syscall_table[SYSCALL_NUMBER_write] = (syscall_t)write;
    syscall_table[SYSCALL_NUMBER_open] = (syscall_t)open;
    syscall_table[SYSCALL_NUMBER_close] = (syscall_t)close;
    syscall_table[SYSCALL_NUMBER_create] = (syscall_t)create;
    syscall_table[SYSCALL_NUMBER_delete] = (syscall_t)delete;
    syscall_table[SYSCALL_NUMBER_getpid] = (syscall_t)getpid;
    syscall_table[SYSCALL_NUMBER_sleep] = (syscall_t)sleep;
}

// Execute a system call
int syscall(syscall_t *sc) {
    // Check if the system call number is valid
    if (sc->number >= SYSCALL_NUMBER_MAX) {
        return -1;
    }

    // Get the address of the system call function
    void (*func)(void) = syscall_table[sc->number].function;

    // Call the system call function
    return func();
}

// Implement the system call functions
int exit(int status) {
    // Release the process's resources
    release_resources(current_process);

    // Terminate the process
    terminate_process(current_process);

    // Return the status to the parent process
    return status;
}

int read(int fd, char *buf, int len) {
    // Check if the file descriptor is valid
    if (!validate_file_descriptor(fd)) {
        return -1;
    }

    // Read data from the file
    int bytes_read = read_file(fd, buf, len);

    // Return the number of bytes read
    return bytes_read;
}

int write(int fd, const char *buf, int len) {
    // Check if the file descriptor is valid
    if (!validate_file_descriptor(fd)) {
        return -1;
    }

    // Write data to the file
    int bytes_written = write_file(fd, buf, len);

    // Return the number of bytes written
    return bytes_written;
}

int open(const char *filename, int flags) {
    // Create a new file object
    file_t *file = create_file(filename, flags);

    // Return the file descriptor
    return file->fd;
}

int close(int fd) {
    // Check if the file descriptor is valid
    if (!validate_file_descriptor(fd)) {
        return -1;
    }

    // Close the file
    close_file(fd);

    // Return success
    return 0;
}

int create(const char *filename, int mode) {
    // Create a new file object
    file_t *file = create_file(filename, mode);

    // Return the file descriptor
    return file->fd;
}

int delete(const char *filename) {
    // Delete the file
    delete_file(filename);

    // Return success
    return 0;
}

int getpid() {
    // Return the current process ID
    return current_process->pid;
}

int sleep(int seconds) {
    // Suspend the process for the specified number of seconds
    suspend_process(seconds);

    // Return success
    return 0;
}
