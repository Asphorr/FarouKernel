#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

// -------------------------------
// Kernel-specific replacements
// -------------------------------
// Mock implementations for kernel functions
void kprintf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

void* kmalloc(size_t size) { return malloc(size); }
void kfree(void* ptr) { free(ptr); }
void ksleep(unsigned int ms) { usleep(ms * 1000); }

typedef unsigned int ktid_t;

// Synchronization primitives
typedef struct {
    atomic_int lock;
} kspinlock;

void spin_lock(kspinlock* lock) {
    while (atomic_exchange_explicit(&lock->lock, 1, memory_order_acquire)) {}
}

void spin_unlock(kspinlock* lock) {
    atomic_store_explicit(&lock->lock, 0, memory_order_release);
}

// -------------------------------
// Configuration Constants
// -------------------------------
#define MAX_PROCESSES 64
#define SCHED_POLICY 0
#define MIN_PRIORITY 1
#define MAX_PRIORITY 99

// -------------------------------
// Logging Mechanism
// -------------------------------
typedef struct {
    kspinlock log_lock;
} Logger;

Logger logger;

void log_message(const char* format, ...) {
    va_list args;
    va_start(args, format);
    spin_lock(&logger.log_lock);
    printf("[KERNEL] ");
    vprintf(format, args);
    printf("\n");
    spin_unlock(&logger.log_lock);
    va_end(args);
}

// -------------------------------
// Process/Thread Information
// -------------------------------
typedef struct {
    ktid_t tid;
    int priority;
    atomic_bool is_running;
    char name[32];
} TaskInfo;

// -------------------------------
// Scheduler Structure
// -------------------------------
typedef struct {
    TaskInfo task_list[MAX_PROCESSES];
    size_t task_count;
    kspinlock task_lock;
    atomic_bool terminate_flag;
} Scheduler;

void initialize_scheduler(Scheduler* scheduler) {
    scheduler->task_count = 0;
    atomic_init(&scheduler->terminate_flag, false);
    atomic_init(&scheduler->task_lock.lock, 0);
    log_message("Scheduler initialized.");
}

// -------------------------------
// Task Management
// -------------------------------
typedef struct {
    Scheduler* scheduler;
    char name[32];
    int priority;
} TaskArgs;

static atomic_uint next_tid = 1;

ktid_t kernel_create_thread(void (*entry)(void*), void* arg, int priority) {
    pthread_t thread;
    ktid_t tid = atomic_fetch_add(&next_tid, 1);
    if (pthread_create(&thread, NULL, (void*(*)(void*))entry, arg) != 0) {
        log_message("ERROR: Thread creation failed");
        return 0;
    }
    pthread_detach(thread);
    return tid;
}

void kernel_set_priority(ktid_t tid, int priority) {
    log_message("Thread %u priority set to %d", tid, priority);
}

void example_task(void* arg) {
    TaskArgs* args = (TaskArgs*)arg;
    Scheduler* sched = args->scheduler;
    log_message("Task %s started", args->name);

    for (int i = 0; i < 5; ++i) {
        log_message("Task %s working... (%d/5)", args->name, i + 1);
        for (volatile int j = 0; j < 1000000; j++) {} // Shorter busy work
    }

    spin_lock(&sched->task_lock);
    for (size_t i = 0; i < sched->task_count; i++) {
        if (strcmp(sched->task_list[i].name, args->name) == 0) {
            atomic_store(&sched->task_list[i].is_running, false);
            break;
        }
    }
    spin_unlock(&sched->task_lock);

    log_message("Task %s completed", args->name);
    kfree(args);
}

void create_task(Scheduler* sched, int priority, const char* name) {
    TaskArgs* args = kmalloc(sizeof(TaskArgs));
    if (!args) {
        log_message("ERROR: Memory allocation failed");
        return;
    }

    strncpy(args->name, name, 31);
    args->name[31] = '\0';
    args->scheduler = sched;
    args->priority = priority;

    ktid_t tid = kernel_create_thread((void(*)(void*))example_task, args, priority);
    if (!tid) {
        kfree(args);
        return;
    }

    spin_lock(&sched->task_lock);
    if (sched->task_count >= MAX_PROCESSES) {
        log_message("ERROR: Task limit reached");
        spin_unlock(&sched->task_lock);
        kfree(args);
        return;
    }

    sched->task_list[sched->task_count] = (TaskInfo){
        .tid = tid,
        .priority = priority,
        .is_running = ATOMIC_VAR_INIT(true),
    };
    strncpy(sched->task_list[sched->task_count].name, name, 31);
    sched->task_list[sched->task_count].name[31] = '\0';
    sched->task_count++;
    spin_unlock(&sched->task_lock);

    log_message("Created task %s (prio %d)", name, priority);
}

// -------------------------------
// Priority Management
// -------------------------------
void priority_manager(Scheduler* sched) {
    while (!atomic_load(&sched->terminate_flag)) {
        ksleep(3000);
        
        spin_lock(&sched->task_lock);
        for (size_t i = 0; i < sched->task_count; i++) {
            if (!atomic_load(&sched->task_list[i].is_running)) continue;

            int new_prio = sched->task_list[i].priority + (rand() % 3 - 1);
            new_prio = new_prio < MIN_PRIORITY ? MIN_PRIORITY :
                      new_prio > MAX_PRIORITY ? MAX_PRIORITY : new_prio;
            
            kernel_set_priority(sched->task_list[i].tid, new_prio);
            sched->task_list[i].priority = new_prio;
            log_message("Updated %s to prio %d", sched->task_list[i].name, new_prio);
        }
        spin_unlock(&sched->task_lock);
    }
}

// -------------------------------
// Main Execution
// -------------------------------
Scheduler global_scheduler;

int main() {
    atomic_init(&logger.log_lock.lock, 0);
    srand(time(NULL));
    
    initialize_scheduler(&global_scheduler);
    kernel_create_thread((void(*)(void*))priority_manager, &global_scheduler, MAX_PRIORITY);

    create_task(&global_scheduler, 90, "HighPrio");
    create_task(&global_scheduler, 50, "MidPrio");
    create_task(&global_scheduler, 30, "LowPrio");

    // Run for 20 seconds
    ksleep(20000);
    atomic_store(&global_scheduler.terminate_flag, true);
    
    // Allow time for cleanup
    ksleep(1000);
    log_message("Main kernel execution completed");
    return 0;
}
