// scheduler.c
#define _GNU_SOURCE
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
#include <stdbool.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>

// -------------------------------
// Configuration Constants
// -------------------------------
#define MAX_PROCESSES 64
#define LOG_FILE "scheduler.log"

// -------------------------------
// Logging Mechanism
// -------------------------------
typedef struct {
    FILE* log_fd;
    pthread_mutex_t log_mutex;
} Logger;

Logger logger;

// Initialize the logger
void init_logger(const char* filename) {
    pthread_mutex_init(&logger.log_mutex, NULL);
    logger.log_fd = fopen(filename, "a");
    if (!logger.log_fd) {
        perror("Failed to open log file");
        exit(EXIT_FAILURE);
    }
}

// Log a message with timestamp
void log_message(const char* format, ...) {
    pthread_mutex_lock(&logger.log_mutex);
    
    // Get current time
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    char time_buffer[26];
    strftime(time_buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
    
    // Write timestamp
    fprintf(logger.log_fd, "[%s] ", time_buffer);
    
    // Write the actual log message
    va_list args;
    va_start(args, format);
    vfprintf(logger.log_fd, format, args);
    va_end(args);
    
    fprintf(logger.log_fd, "\n");
    fflush(logger.log_fd);
    pthread_mutex_unlock(&logger.log_mutex);
}

// Close the logger
void close_logger() {
    if (logger.log_fd) {
        fclose(logger.log_fd);
    }
    pthread_mutex_destroy(&logger.log_mutex);
}

// -------------------------------
// Process/Thread Information
// -------------------------------
typedef struct {
    pid_t pid;                    // Process ID (0 for threads)
    int priority;                 // Priority (higher value = higher priority)
    pthread_t thread;             // Thread ID (valid if is_thread is true)
    bool is_thread;               // Flag indicating if it's a thread
    atomic_bool is_running;       // Atomic flag indicating if it's running
    char name[32];                // Name for identification
} ProcessInfo;

// -------------------------------
// Scheduler Structure
// -------------------------------
typedef struct {
    ProcessInfo process_list[MAX_PROCESSES];
    size_t process_count;
    pthread_mutex_t process_mutex;
    pthread_cond_t process_cv;
    atomic_bool terminate_flag;
} Scheduler;

// Initialize the scheduler
void initialize_scheduler(Scheduler* scheduler) {
    scheduler->process_count = 0;
    pthread_mutex_init(&scheduler->process_mutex, NULL);
    pthread_cond_init(&scheduler->process_cv, NULL);
    atomic_store(&scheduler->terminate_flag, false);
    log_message("Scheduler initialized.");
}

// Cleanup the scheduler
void cleanup_scheduler(Scheduler* scheduler) {
    pthread_mutex_destroy(&scheduler->process_mutex);
    pthread_cond_destroy(&scheduler->process_cv);
    log_message("Scheduler cleaned up.");
}

// -------------------------------
// Utility Functions
// -------------------------------

// Handle system call errors
void handle_syscall_error(const char* message) {
    log_message("ERROR: %s: %s", message, strerror(errno));
    exit(EXIT_FAILURE);
}

// Handle pthread errors
void handle_pthread_error(int err_code, const char* message) {
    if (err_code != 0) {
        log_message("ERROR: %s: %s", message, strerror(err_code));
        exit(EXIT_FAILURE);
    }
}

// Compare function for priority sorting (higher priority first)
int compare_priority(const void* a, const void* b) {
    ProcessInfo* procA = (ProcessInfo*)a;
    ProcessInfo* procB = (ProcessInfo*)b;
    return procB->priority - procA->priority;
}

// Add a new process/thread to the scheduler
void add_process_info(Scheduler* scheduler, pid_t pid, int priority, bool is_thread, pthread_t thread, const char* name) {
    pthread_mutex_lock(&scheduler->process_mutex);
    if (scheduler->process_count >= MAX_PROCESSES) {
        pthread_mutex_unlock(&scheduler->process_mutex);
        log_message("ERROR: Maximum process limit reached.");
        exit(EXIT_FAILURE);
    }

    ProcessInfo* proc = &scheduler->process_list[scheduler->process_count];
    proc->pid = pid;
    proc->priority = priority;
    proc->thread = thread;
    proc->is_thread = is_thread;
    atomic_store(&proc->is_running, true);
    strncpy(proc->name, name ? name : "Unnamed", sizeof(proc->name) - 1);
    proc->name[sizeof(proc->name) - 1] = '\0';
    scheduler->process_count++;
    pthread_cond_signal(&scheduler->process_cv); // Notify scheduler
    pthread_mutex_unlock(&scheduler->process_mutex);
    log_message("Added %s: %s (PID: %d, Priority: %d)", 
                is_thread ? "Thread" : "Process", 
                proc->name, 
                is_thread ? 0 : pid, 
                priority);
}

// Remove a process/thread from the scheduler
void remove_process_info(Scheduler* scheduler, pid_t pid, pthread_t thread_id) {
    pthread_mutex_lock(&scheduler->process_mutex);
    for (size_t i = 0; i < scheduler->process_count; ++i) {
        if ((scheduler->process_list[i].pid == pid && !scheduler->process_list[i].is_thread) ||
            (scheduler->process_list[i].is_thread && pthread_equal(scheduler->process_list[i].thread, thread_id))) {
            log_message("Removing %s: %s (PID: %d)", 
                        scheduler->process_list[i].is_thread ? "Thread" : "Process",
                        scheduler->process_list[i].name,
                        scheduler->process_list[i].is_thread ? 0 : scheduler->process_list[i].pid);
            // Swap with the last process and decrease count
            scheduler->process_list[i] = scheduler->process_list[scheduler->process_count - 1];
            scheduler->process_count--;
            break;
        }
    }
    pthread_mutex_unlock(&scheduler->process_mutex);
}

// -------------------------------
// Task Management Functions
// -------------------------------

// Example function for processes/threads
void example_function(void) {
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
    log_message("Task %d started.", getpid());

    for (int i = 0; i < 5; ++i) {
        log_message("Task %d working... (%d/5)", getpid(), i + 1);
        sleep(1);
        // Example of dynamic priority adjustment
        // In a real scenario, this could be based on task progress or external inputs
    }

    log_message("Task %d completed.", getpid());
    // For threads, we need to ensure they remove themselves from the scheduler
    // This will be handled in the thread wrapper
}

// Wrapper for thread functions to handle cleanup
typedef struct {
    Scheduler* scheduler;
    void (*func)(void);
    char name[32];
} ThreadArgs;

void* thread_wrapper(void* args) {
    ThreadArgs* thread_args = (ThreadArgs*)args;
    Scheduler* scheduler = thread_args->scheduler;
    void (*func)(void) = thread_args->func;
    char name[32];
    strncpy(name, thread_args->name, sizeof(name) - 1);
    name[sizeof(name) - 1] = '\0';
    free(thread_args); // Free allocated memory

    // Execute the actual function
    func();

    // After function completes, remove the thread from the scheduler
    remove_process_info(scheduler, 0, pthread_self());
    log_message("Thread %s exiting.", name);
    return NULL;
}

// Create a new process
pid_t create_process(Scheduler* scheduler, void (*func)(void), int priority, const char* name) {
    pid_t pid = fork();
    if (pid < 0) {
        handle_syscall_error("Failed to fork process");
    } else if (pid == 0) {
        // Child process
        func();
        _exit(EXIT_SUCCESS);
    }

    // Parent process
    add_process_info(scheduler, pid, priority, false, 0, name);
    return pid;
}

// Create a new thread
void create_thread(Scheduler* scheduler, void (*func)(void), int priority, const char* name) {
    pthread_t thread;
    ThreadArgs* args = malloc(sizeof(ThreadArgs));
    if (!args) {
        handle_syscall_error("Failed to allocate memory for thread arguments");
    }
    args->scheduler = scheduler;
    args->func = func;
    strncpy(args->name, name ? name : "Unnamed", sizeof(args->name) - 1);
    args->name[sizeof(args->name) - 1] = '\0';

    int ret = pthread_create(&thread, NULL, thread_wrapper, args);
    if (ret != 0) {
        free(args);
        handle_pthread_error(ret, "Failed to create thread");
    }

    add_process_info(scheduler, 0, priority, true, thread, name);
}

// Wait for a specific process to finish
int wait_for_process(Scheduler* scheduler, pid_t pid) {
    int status;
    if (waitpid(pid, &status, 0) == -1) {
        log_message("ERROR: Failed to wait for process %d: %s", pid, strerror(errno));
        return -1;
    }
    remove_process_info(scheduler, pid, 0);
    log_message("Process %d exited with status %d.", pid, WEXITSTATUS(status));
    return status;
}

// Wait for all processes and threads to finish
void wait_all_processes(Scheduler* scheduler) {
    pthread_mutex_lock(&scheduler->process_mutex);
    while (scheduler->process_count > 0) {
        for (size_t i = 0; i < scheduler->process_count;) {  // Note: process_count may decrease
            ProcessInfo proc = scheduler->process_list[i];
            if (proc.is_thread) {
                pthread_mutex_unlock(&scheduler->process_mutex);
                int ret = pthread_join(proc.thread, NULL);
                if (ret != 0) {
                    log_message("ERROR: Failed to join thread %s: %s", proc.name, strerror(ret));
                }
                pthread_mutex_lock(&scheduler->process_mutex);
                // Removing the thread is handled in the thread wrapper
            } else {
                pthread_mutex_unlock(&scheduler->process_mutex);
                wait_for_process(scheduler, proc.pid);
                pthread_mutex_lock(&scheduler->process_mutex);
                // After wait_for_process, the process is removed, so don't increment i
                continue;
            }
            i++;
        }
    }
    pthread_mutex_unlock(&scheduler->process_mutex);
}

// Terminate all processes and threads
void terminate_all_processes(Scheduler* scheduler) {
    pthread_mutex_lock(&scheduler->process_mutex);
    log_message("Terminating all processes and threads.");
    for (size_t i = 0; i < scheduler->process_count; ++i) {
        ProcessInfo* proc = &scheduler->process_list[i];
        if (proc->is_thread) {
            pthread_cancel(proc->thread);
            log_message("Sent cancellation to thread %s.", proc->name);
        } else {
            if (kill(proc->pid, SIGTERM) == -1) {
                log_message("ERROR: Failed to send SIGTERM to process %d: %s", proc->pid, strerror(errno));
            } else {
                log_message("Sent SIGTERM to process %d.", proc->pid);
            }
        }
    }
    pthread_mutex_unlock(&scheduler->process_mutex);
    wait_all_processes(scheduler);
}

// -------------------------------
// Signal Handling
// -------------------------------

// Forward declaration of scheduler for the signal handler
Scheduler global_scheduler;

// Signal handler for graceful termination
void signal_handler(int signum) {
    log_message("Received signal %d, initiating graceful termination.", signum);
    atomic_store(&global_scheduler.terminate_flag, true);
    terminate_all_processes(&global_scheduler);
    cleanup_scheduler(&global_scheduler);
    close_logger();
    exit(EXIT_SUCCESS);
}

// Set up signal handlers
void setup_signal_handlers() {
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    // Handle SIGINT and SIGTERM
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        handle_syscall_error("Failed to set SIGINT handler");
    }
    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        handle_syscall_error("Failed to set SIGTERM handler");
    }
}

// -------------------------------
// Scheduler Loop
// -------------------------------

void schedule_round_robin(Scheduler* scheduler) {
    log_message("Scheduler loop started.");
    while (!atomic_load(&scheduler->terminate_flag)) {
        pthread_mutex_lock(&scheduler->process_mutex);
        while (scheduler->process_count == 0 && !atomic_load(&scheduler->terminate_flag)) {
            pthread_cond_wait(&scheduler->process_cv, &scheduler->process_mutex);
        }
        if (atomic_load(&scheduler->terminate_flag)) {
            pthread_mutex_unlock(&scheduler->process_mutex);
            break;
        }

        // Sort the process list based on priority
        qsort(scheduler->process_list, scheduler->process_count, sizeof(ProcessInfo), compare_priority);

        // Clone the current process list for iteration to avoid holding the mutex
        ProcessInfo current_processes[MAX_PROCESSES];
        size_t current_count = scheduler->process_count;
        memcpy(current_processes, scheduler->process_list, sizeof(ProcessInfo) * current_count);
        pthread_mutex_unlock(&scheduler->process_mutex);

        // Iterate through the sorted process list
        for (size_t i = 0; i < current_count; ++i) {
            if (atomic_load(&current_processes[i].is_running)) {
                if (current_processes[i].is_thread) {
                    // For threads, check if they are still running
                    int ret = pthread_kill(current_processes[i].thread, 0);
                    if (ret == ESRCH) { // Thread no longer exists
                        remove_process_info(scheduler, 0, current_processes[i].thread);
                        continue;
                    } else if (ret != 0) {
                        log_message("ERROR: Failed to check thread %s status: %s", 
                                    current_processes[i].name, strerror(ret));
                        continue;
                    }
                    // For demonstration, just log the running thread
                    log_message("Running Thread: %s (Priority: %d)", 
                                current_processes[i].name, current_processes[i].priority);
                } else {
                    // For processes, check if they are still running
                    if (kill(current_processes[i].pid, 0) == -1) {
                        if (errno == ESRCH) {
                            // Process no longer exists
                            remove_process_info(scheduler, current_processes[i].pid, 0);
                            continue;
                        } else {
                            log_message("ERROR: Failed to check process %d status: %s", 
                                        current_processes[i].pid, strerror(errno));
                            continue;
                        }
                    }
                    // For demonstration, just log the running process
                    log_message("Running Process: %s (PID: %d, Priority: %d)", 
                                current_processes[i].name, current_processes[i].pid, current_processes[i].priority);
                }
            }
        }

        // Sleep to simulate time slicing
        struct timespec ts = { .tv_sec = 1, .tv_nsec = 0 };
        nanosleep(&ts, NULL);
    }
    log_message("Scheduler loop terminated.");
}

// -------------------------------
// Dynamic Priority Adjustment
// -------------------------------

// Example function to dynamically adjust priorities
void* priority_manager(void* arg) {
    Scheduler* scheduler = (Scheduler*)arg;
    int counter = 0;
    while (!atomic_load(&scheduler->terminate_flag)) {
        sleep(5); // Adjust priorities every 5 seconds
        pthread_mutex_lock(&scheduler->process_mutex);
        for (size_t i = 0; i < scheduler->process_count; ++i) {
            // Example: Decrease priority of processes/threads that have been running for a while
            scheduler->process_list[i].priority += (counter % 2 == 0) ? 1 : -1;
            if (scheduler->process_list[i].priority < 1) {
                scheduler->process_list[i].priority = 1;
            }
            log_message("Adjusted priority of %s: %s (New Priority: %d)",
                        scheduler->process_list[i].is_thread ? "Thread" : "Process",
                        scheduler->process_list[i].name,
                        scheduler->process_list[i].priority);
        }
        pthread_mutex_unlock(&scheduler->process_mutex);
        counter++;
    }
    return NULL;
}

// -------------------------------
// Main Function
// -------------------------------

int main(int argc, char* argv[]) {
    // Initialize logger
    init_logger(LOG_FILE);
    log_message("Scheduler starting.");

    // Initialize the scheduler
    initialize_scheduler(&global_scheduler);

    // Set up signal handlers
    setup_signal_handlers();

    // Create a priority manager thread for dynamic priority adjustments
    pthread_t priority_thread;
    int ret = pthread_create(&priority_thread, NULL, priority_manager, &global_scheduler);
    handle_pthread_error(ret, "Failed to create priority manager thread");

    // Create some example processes and threads
    create_process(&global_scheduler, example_function, 3, "Process1");
    create_process(&global_scheduler, example_function, 2, "Process2");
    create_thread(&global_scheduler, example_function, 4, "Thread1");
    create_thread(&global_scheduler, example_function, 1, "Thread2");

    // Start the scheduler loop
    schedule_round_robin(&global_scheduler);

    // Wait for the priority manager to finish
    pthread_join(priority_thread, NULL);

    // Wait for all processes and threads to finish
    wait_all_processes(&global_scheduler);

    // Cleanup the scheduler
    cleanup_scheduler(&global_scheduler);

    // Close the logger
    close_logger();

    log_message("Scheduler exited successfully.");
    return 0;
}
