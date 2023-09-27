#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kernel.h"

// Initialize the kernel
void kernel_init(void) {
    // Initialize the heap
    heap = (void*)ALIGN(HEAP_SIZE, PAGE_SIZE);
    heap_size = HEAP_SIZE;

    // Initialize the idle thread
    idletask = (struct thread*)kmalloc(sizeof(struct thread));
    idletask->func = idle;
    idletask->next = NULL;

    // Initialize the system call table
    syscall_table[SYS_EXIT] = exit;
    syscall_table[SYS_CREATE_PROCESS] = create_process;
    syscall_table[SYS_CREATE_THREAD] = create_thread;
    syscall_table[SYS_YIELD] = yield;
    syscall_table[SYS_SLEEP] = sleep;
    syscall_table[SYS_WAKEUP] = wakeup;
    syscall_table[SYS_TERMINATE_PROCESS] = terminate_process;
    syscall_table[SYS_FREE_MEMORY] = free_memory;

    // Enable interrupts
    enable_interrupts();
}

// Handle a system call
void kernel_handle_syscall(struct thread *thread, uint num) {
    switch (num) {
        case SYS_EXIT:
            exit(thread);
            break;
        case SYS_CREATE_PROCESS:
            create_process(thread);
            break;
        case SYS_CREATE_THREAD:
            create_thread(thread);
            break;
        case SYS_YIELD:
            yield(thread);
            break;
        case SYS_SLEEP:
            sleep(thread, 1000);
            break;
        case SYS_WAKEUP:
            wakeup(thread);
            break;
        case SYS_TERMINATE_PROCESS:
            terminate_process(thread);
            break;
        case SYS_FREE_MEMORY:
            free_memory(thread);
            break;
        default:
            printf("Unknown system call %d\n", num);
            break;
    }
}

// Idle loop
void idle(void) {
    while (1) {
        // Check for incoming system calls
        if (syscall_flag) {
            syscall_flag = 0;
            kernel_handle_syscall(idletask, syscall_number);
        }

        // Yield control back to the OS
        yield(idletask);
    }
}

// Create a new process
void create_process(struct thread *thread) {
    // Allocate memory for the process's stack and image
    struct memory_region *region = (struct memory_region*)kmalloc(sizeof(struct memory_region));
    region->base = (void*)kmalloc(STACK_SIZE + IMAGE_SIZE);
    region->size = STACK_SIZE + IMAGE_SIZE;

    // Set up the process's context
    struct process *process = (struct process*)kmalloc(sizeof(struct process));
    process->entry = thread->func;
    process->threads = NULL;
    process->pid = get_pid();

    // Add the process to the list of active processes
    process->next = active_processes;
    active_processes = process;

    // Start the process
    start_process(process);
}

// Create a new thread
void create_thread(struct thread *thread) {
    // Allocate memory for the thread's stack
    struct memory_region *region = (struct memory_region*)kmalloc(sizeof(struct memory_region));
    region->base = (void*)kmalloc(STACK_SIZE);
    region->size = STACK_SIZE;

    // Set up the thread's context
    struct thread *new_thread = (struct thread*)kmalloc(sizeof(struct thread));
    new_thread->func = thread->func;
    new_thread->next = NULL;

    // Add the thread to the list of ready threads
ready_threads = append(ready_threads, new_thread);

// Switch to the new thread
switch_to(new_thread);
