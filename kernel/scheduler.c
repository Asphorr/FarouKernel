// scheduler.c
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

// Define maximum number of processes/threads
#define MAX_PROCESSES 64

// Structure to store process/thread information
typedef struct {
    pid_t pid;                    // Process ID (0 for threads)
    int priority;                 // Priority of the process/thread
    pthread_t thread;             // Thread ID (valid if is_thread is true)
    bool is_thread;               // Flag indicating if it's a thread
    atomic_bool is_running;       // Atomic flag indicating if it's running
    char name[32];                // Optional name for identification
} ProcessInfo;

// Global variables for process management
static ProcessInfo process_list[MAX_PROCESSES];
static size_t process_count = 0;
static pthread_mutex_t process_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t process_cv = PTHREAD_COND_INITIALIZER;
static volatile sig_atomic_t terminate_flag = 0;

// Utility function to handle syscall errors
static void handle_syscall_error(const char* message) {
    perror(message);
    exit(EXIT_FAILURE);
}

// Utility function to handle pthread errors with error code and message
static void handle_pthread_error(int err_code, const char* message) {
    if (err_code != 0) {
        fprintf(stderr, "%s: %s\n", message, strerror(err_code));
        exit(EXIT_FAILURE);
    }
}

// Add a new process/thread to the process list
static void add_process_info(pid_t pid, int priority, bool is_thread, pthread_t thread, const char* name) {
    pthread_mutex_lock(&process_mutex);
    if (process_count >= MAX_PROCESSES) {
        pthread_mutex_unlock(&process_mutex);
        fprintf(stderr, "Max process limit reached\n");
        exit(EXIT_FAILURE);
    }

    ProcessInfo* proc = &process_list[process_count];
    proc->pid = pid;
    proc->priority = priority;
    proc->thread = thread;
    proc->is_thread = is_thread;
    atomic_store(&proc->is_running, true);
    strncpy(proc->name, name ? name : "Unnamed", sizeof(proc->name) - 1);
    proc->name[sizeof(proc->name) - 1] = '\0';
    process_count++;
    pthread_cond_signal(&process_cv); // Signal scheduler that a new process/thread is added
    pthread_mutex_unlock(&process_mutex);
}

// Remove a process/thread from the process list
static void remove_process_info(pid_t pid, pthread_t thread_id) {
    pthread_mutex_lock(&process_mutex);
    for (size_t i = 0; i < process_count; ++i) {
        if ((process_list[i].pid == pid && !process_list[i].is_thread) ||
            (process_list[i].is_thread && pthread_equal(process_list[i].thread, thread_id))) {
            // Swap with the last process and reduce the count
            process_list[i] = process_list[process_count - 1];
            process_count--;
            break;
        }
    }
    pthread_mutex_unlock(&process_mutex);
}

// Create a new process
pid_t create_process(void (*func)(void), int priority, const char* name) {
    pid_t pid = fork();
    if (pid == -1) {
        handle_syscall_error("Failed to create process");
    } else if (pid == 0) {
        // Child process
        func();
        _exit(EXIT_SUCCESS); // Use _exit to avoid flushing stdio buffers
    }

    // Parent process
    add_process_info(pid, priority, false, 0, name);
    return pid;
}

// Wrapper function for thread execution to handle cleanup
static void* thread_wrapper(void* arg) {
    void (*func)(void) = ((void (**)(void))arg)[0];
    char* thread_name = ((char**)arg)[1];

    func();

    // After thread function completes, remove from process list
    remove_process_info(0, pthread_self());
    return NULL;
}

// Create a new thread
void create_thread(void (*func)(void), int priority, const char* name) {
    pthread_t thread;
    // Allocate memory to pass multiple arguments to thread_wrapper
    void** args = malloc(2 * sizeof(void*));
    if (!args) {
        handle_syscall_error("Failed to allocate memory for thread arguments");
    }
    args[0] = func;
    args[1] = (char*)name;

    int ret = pthread_create(&thread, NULL, thread_wrapper, args);
    if (ret != 0) {
        free(args);
        handle_pthread_error(ret, "Failed to create thread");
    }

    add_process_info(0, priority, true, thread, name);
}

// Wait for a specific process to finish
int wait_for_process(pid_t pid) {
    int status;
    if (waitpid(pid, &status, 0) == -1) {
        fprintf(stderr, "Failed to wait for process %d: %s\n", pid, strerror(errno));
        return -1;
    }
    remove_process_info(pid, 0);
    return status;
}

// Wait for all processes/threads to finish
void wait_all_processes() {
    pthread_mutex_lock(&process_mutex);
    while (process_count > 0) {
        for (size_t i = 0; i < process_count; ) {  // Note: process_count may decrease
            ProcessInfo proc = process_list[i];
            if (proc.is_thread) {
                pthread_mutex_unlock(&process_mutex);
                void* res;
                int ret = pthread_join(proc.thread, &res);
                pthread_mutex_lock(&process_mutex);
                if (ret != 0) {
                    fprintf(stderr, "Failed to join thread %s: %s\n", proc.name, strerror(ret));
                }
                remove_process_info(0, proc.thread);
            } else {
                pthread_mutex_unlock(&process_mutex);
                int status = wait_for_process(proc.pid);
                pthread_mutex_lock(&process_mutex);
                (void)status; // Optionally handle status
            }
        }
    }
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
    pthread_mutex_unlock(&process_mutex);
    wait_all_processes(); // Ensure all are terminated
}

// Initialize the scheduler
void initialize_scheduler() {
    handle_pthread_error(pthread_mutex_init(&process_mutex, NULL), "Failed to initialize mutex");
    handle_pthread_error(pthread_cond_init(&process_cv, NULL), "Failed to initialize condition variable");
}

// Cleanup the scheduler
void cleanup_scheduler() {
    handle_pthread_error(pthread_mutex_destroy(&process_mutex), "Failed to destroy mutex");
    handle_pthread_error(pthread_cond_destroy(&process_cv), "Failed to destroy condition variable");
}

// Signal handler for graceful termination
static void signal_handler(int signum) {
    (void)signum; // Unused parameter
    terminate_flag = 1;
    terminate_all_processes();
    cleanup_scheduler();
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

// Compare function for priority sorting (higher priority first)
static int compare_priority(const void* a, const void* b) {
    ProcessInfo* procA = (ProcessInfo*)a;
    ProcessInfo* procB = (ProcessInfo*)b;
    return procB->priority - procA->priority;
}

// Round-robin (priority-based) scheduler
void schedule_round_robin() {
    while (!terminate_flag) {
        pthread_mutex_lock(&process_mutex);
        while (process_count == 0 && !terminate_flag) {
            pthread_cond_wait(&process_cv, &process_mutex);
        }

        if (terminate_flag) {
            pthread_mutex_unlock(&process_mutex);
            break;
        }

        // Sort processes based on priority
        qsort(process_list, process_count, sizeof(ProcessInfo), compare_priority);

        // Iterate through the processes based on priority
        for (size_t i = 0; i < process_count; ++i) {
            ProcessInfo proc = process_list[i];
            if (atomic_load(&proc.is_running)) {
                if (proc.is_thread) {
                    // For threads, you can implement thread-specific scheduling logic here
                    // For simplicity, just check if thread is still running
                    int ret = pthread_kill(proc.thread, 0);
                    if (ret == ESRCH) { // Thread no longer exists
                        remove_process_info(0, proc.thread);
                        continue;
                    } else if (ret != 0) {
                        fprintf(stderr, "Failed to check thread status %s: %s\n", proc.name, strerror(ret));
                        continue;
                    }
                } else {
                    // For processes, check if the process is still running
                    if (kill(proc.pid, 0) == -1 && errno == ESRCH) {
                        // Process no longer exists
                        remove_process_info(proc.pid, 0);
                        continue;
                    } else if (kill(proc.pid, 0) == -1) {
                        fprintf(stderr, "Failed to check process status %d: %s\n", proc.pid, strerror(errno));
                        continue;
                    }
                }
                // Implement actual scheduling (e.g., signal processes/threads to run)
                // For demonstration, we'll just print the running processes
                printf("Running %s (PID: %d, Priority: %d)\n",
                       proc.is_thread ? "Thread" : "Process",
                       proc.is_thread ? 0 : proc.pid,
                       proc.priority);
            }
        }
        pthread_mutex_unlock(&process_mutex);

        // Time slice duration
        struct timespec ts = { .tv_sec = 1, .tv_nsec = 0 };
        nanosleep(&ts, NULL);
    }
}

// Example function for a process/thread
void example_function(void) {
    printf("Example function is running in thread/process %d\n", getpid());
    for (int i = 0; i < 5; ++i) {
        printf("Process/Thread %d working... (%d)\n", getpid(), i + 1);
        sleep(1);
    }
    printf("Example function finished in thread/process %d\n", getpid());
}

// Main function to demonstrate the scheduler usage
int main() {
    // Initialize the scheduler
    initialize_scheduler();

    // Set signal handler for graceful termination
    set_signal_handler(SIGINT, signal_handler);
    set_signal_handler(SIGTERM, signal_handler);

    // Create processes and threads
    create_process(example_function, 2, "Process1");
    create_process(example_function, 1, "Process2");
    create_thread(example_function, 3, "Thread1");
    create_thread(example_function, 2, "Thread2");

    // Start round-robin scheduler
    schedule_round_robin();

    // Wait for all processes/threads to finish
    wait_all_processes();

    // Cleanup resources
    cleanup_scheduler();
    return 0;
}
