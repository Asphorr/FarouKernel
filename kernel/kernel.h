#include <stddef.h>
#include <stdbool.h>
#include <string.h>

#define THREAD_NULL ((struct thread *)0x0)
#define PROCESS_NULL ((struct process *)0x0)
#define MEMORY_REGION_NULL ((struct memory_region *)0x0)

typedef unsigned int uint;
typedef signed int sint;
typedef unsigned char uchar;
typedef signed char schar;
typedef float fp;
typedef double dp;

struct thread {
    void (*func)(void);
    struct thread *next;
};

struct process {
    void (*entry)(void);
    struct thread *threads;
    uint pid;
};

struct memory_region {
    void *base;
    size_t size;
};

static inline void kernel_init() {}
static inline void kernel_exit() {}

static inline int kernel_create_process(void (*entry)(void)) {
    struct process *process = malloc(sizeof(*process));
    if (!process) {
        return -1;
    }
    process->entry = entry;
    process->threads = NULL;
    process->pid = 0;
    return 0;
}

static inline int kernel_create_thread(void (*func)(void)) {
    struct thread *thread = malloc(sizeof(*thread));
    if (!thread) {
        return -1;
    }
    thread->func = func;
    thread->next = NULL;
    return 0;
}

static inline void kernel_yield() {}

static inline void kernel_sleep(uint milliseconds) {}

static inline void kernel_wakeup(struct thread *thread) {}

static inline void kernel_terminate_process(struct process *process) {
    free(process);
}

static inline void kernel_free_memory(struct memory_region *region) {
    free(region->base);
}

static inline void* kmalloc(size_t size) {
    void* ptr = NULL;
    if (size > 0) {
        ptr = (void*)((uintptr_t)heap + heap_size - size);
        heap_size -= size;
    }
    return ptr;
}

static inline void kfree(void* ptr) {
    if (ptr != NULL) {
        size_t size = (size_t)((uintptr_t)ptr - (uintptr_t)heap);
        heap_size += size;
        memset(ptr, 0, size);
    }
}

static inline void kio(void* buffer, size_t size, bool is_read) {
    // Todo: Implement I/O operations here
}
