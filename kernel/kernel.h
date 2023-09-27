#ifndef _KERNEL_H
#define _KERNEL_H

// Data Types
typedef unsigned int uint;
typedef signed int sint;
typedef unsigned char uchar;
typedef signed char schar;
typedef float fp;
typedef double dp;

// Basic Structures
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

// Function Prototypes
void kernel_init(void);
void kernel_exit(void);
int kernel_create_process(void (*entry)(void));
int kernel_create_thread(void (*func)(void));
void kernel_yield(void);
void kernel_sleep(uint milliseconds);
void kernel_wakeup(struct thread *thread);
void kernel_terminate_process(struct process *process);
void kernel_free_memory(struct memory_region *region);

// Macros
#define THREAD_NULL ((struct thread *)0x0)
#define PROCESS_NULL ((struct process *)0x0)
#define MEMORY_REGION_NULL ((struct memory_region *)0x0)

#endif // _KERNEL_H

void* kmalloc(size_t size) {
    void* ptr = NULL;
    if (size > 0) {
        ptr = (void*)((uintptr_t)heap + heap_size - size);
        heap_size -= size;
    }
    return ptr;
}
void kfree(void* ptr) {
    if (ptr != NULL) {
        size_t size = (size_t)((uintptr_t)ptr - (uintptr_t)heap);
        heap_size += size;
        memset(ptr, 0, size);
    }
}
void kio(void* buffer, size_t size, bool is_read) {
    // Todo: Implement I/O operations here
}
