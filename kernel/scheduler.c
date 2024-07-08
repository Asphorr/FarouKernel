#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdatomic.h>
#include "scheduler.h"

#define MAX_PROCESSES 64

// Structure to store process information
typedef struct {
    pid_t pid;
    int priority;
    pthread_t thread;
    bool is_thread;
    atomic_bool is_running;
} ProcessInfo;

// Global variables for process management
static ProcessInfo process_list[MAX_PROCESSES];
static size_t process_count = 0;
static pthread_mutex_t process_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t process_cv = PTHREAD_COND_INITIALIZER;

// Utility function to handle syscall errors
static void handle_syscall_error(const char* message) {
    perror(message);
    exit(EXIT_FAILURE);
}

// Add a new process to the process list
static void add_process_info(pid_t pid, int priority, bool is_thread, pthread_t thread) {
    pthread_mutex_lock(&process_mutex);
    if (process_count >= MAX_PROCESSES) {
        pthread_mutex_unlock(&process_mutex);
        fprintf(stderr, "Max process limit reached\n");
        exit(EXIT_FAILURE);
    }

    process_list[process_count].pid = pid;
    process_list[process_count].priority = priority;
    process_list[process_count].thread = thread;
    process_list[process_count].is_thread = is_thread;
    atomic_store(&process_list[process_count].is_running, true);
    process_count++;
    pthread_mutex_unlock(&process_mutex);
}

// Remove a process from the process list
static void remove_process_info(pid_t pid) {
    pthread_mutex_lock(&process_mutex);
    for (size_t i = 0; i < process_count; ++i) {
        if (process_list[i].pid == pid || (process_list[i].is_thread && process_list[i].thread == pthread_self())) {
            process_list[i] = process_list[--process_count];
            break;
        }
    }
    pthread_mutex_unlock(&process_mutex);
}

// Create a new process
pid_t create_process(void (*func)(void), int priority) {
    pid_t pid = fork();
    if (pid == -1) {
        handle_syscall_error("Failed to create process");
    } else if (pid == 0) {
        // Child process
        func();
        exit(EXIT_SUCCESS);
    }

    // Parent process
    add_process_info(pid, priority, false, 0);
    return pid;
}

// Create a new thread
void create_thread(void (*func)(void), int priority) {
    pthread_t thread;
    int ret = pthread_create(&thread, NULL, (void* (*)(void*))func, NULL);
    if (ret != 0) {
        handle_syscall_error("Failed to create thread");
    }

    add_process_info(0, priority, true, thread);
}

// Wait for a specific process to finish
int wait_for_process(pid_t pid) {
    int status;
    if (waitpid(pid, &status, 0) == -1) {
        handle_syscall_error("Failed to wait for process");
    }
    return status;
}

// Wait for all processes/threads to finish
void wait_all_processes() {
    pthread_mutex_lock(&process_mutex);
    while (process_count > 0) {
        for (size_t i = 0; i < process_count; ++i) {
            if (process_list[i].is_thread) {
                if (pthread_join(process_list[i].thread, NULL) != 0) {
                    handle_syscall_error("Failed to join thread");
                }
            } else {
                wait_for_process(process_list[i].pid);
            }

            // Mark as not running
            atomic_store(&process_list[i].is_running, false);
        }
        process_count = 0;
    }
    pthread_cond_broadcast(&process_cv);
    pthread_mutex_unlock(&process_mutex);
}

// Terminate all processes/threads
void terminate_all_processes() {
    pthread_mutex_lock(&process_mutex);
    for (size_t i = 0; i < process_count; ++i) {
        if (process_list[i].is_thread) {
            pthread_cancel(process_list[i].thread);
        } else {
            kill(process_list[i].pid, SIGTERM);
        }
    }
    process_count = 0;
    pthread_cond_broadcast(&process_cv);
    pthread_mutex_unlock(&process_mutex);
}

// Initialize the scheduler
void initialize_scheduler() {
    pthread_mutex_init(&process_mutex, NULL);
    pthread_cond_init(&process_cv, NULL);
}

// Cleanup the scheduler
void cleanup_scheduler() {
    pthread_mutex_destroy(&process_mutex);
    pthread_cond_destroy(&process_cv);
}

// Utility function to handle errors in pthread functions
static void handle_pthread_error(int err_code, const char* message) {
    if (err_code != 0) {
        fprintf(stderr, "%s: %s\n", message, strerror(err_code));
        exit(EXIT_FAILURE);
    }
}

// Signal handler for graceful termination
static void signal_handler(int signum) {
    terminate_all_processes();
    exit(EXIT_SUCCESS);
}

// Set signal handler
void set_signal_handler(int signal, void (*handler)(int)) {
    struct sigaction sa;
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(signal, &sa, NULL) == -1) {
        handle_syscall_error("Failed to set signal handler");
    }
}

// Round-robin scheduler
void schedule_round_robin() {
    while (1) {
        pthread_mutex_lock(&process_mutex);
        if (process_count == 0) {
            pthread_cond_wait(&process_cv, &process_mutex);
        }

        for (size_t i = 0; i < process_count; ++i) {
            if (atomic_load(&process_list[i].is_running)) {
                if (process_list[i].is_thread) {
                    int ret = pthread_kill(process_list[i].thread, 0);
                    if (ret != 0 && ret != ESRCH) {
                        handle_pthread_error(ret, "Failed to check thread status");
                    }
                } else {
                    if (kill(process_list[i].pid, 0) == -1 && errno != ESRCH) {
                        handle_syscall_error("Failed to check process status");
                    }
                }
            }
        }
        pthread_mutex_unlock(&process_mutex);

        // A small sleep to simulate time slicing in round-robin scheduling
        sleep(1);
    }
}

// Example function for a process/thread
void example_function() {
    printf("Example function is running\n");
    sleep(2); // Simulate some work
    printf("Example function finished\n");
}

// Main function to demonstrate the scheduler usage
int main() {
    initialize_scheduler();
    set_signal_handler(SIGINT, signal_handler);

    // Create processes and threads
    create_process(example_function, 1);
    create_thread(example_function, 1);

    // Start round-robin scheduler
    schedule_round_robin();

    // Cleanup resources
    cleanup_scheduler();
    return 0;
}
