#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "processes.h"

// Represents a process running on the system
struct process {
    char *name;                   // Name of the process
    int pid;                      // Process ID
    struct proc_state state;     // State of the process (e.g., running, sleeping, stopped)
    struct process *next;       // Pointer to the next process in the list
};

// Represents the current state of a process
struct proc_state {
    int running;                // True if the process is currently running
    int sleeping;               // True if the process is sleeping
    int stopped;                // True if the process has been stopped
};

// Initializes the process manager
void init_process_manager() {
    // Create an empty list of processes
    struct process *process_list = NULL;

    // Set up the first process (the idle process)
    struct process *idle_process = malloc(sizeof(struct process));
    idle_process->name = "Idle";
    idle_process->pid = 0;
    idle_process->state.running = true;
    idle_process->state.sleeping = false;
    idle_process->state.stopped = false;
    process_list = idle_process;

    // Set up the second process (the processor)
    struct process *processor_process = malloc(sizeof(struct process));
    processor_process->name = "Processor";
    processor_process->pid = 1;
    processor_process->state.running = true;
    processor_process->state.sleeping = false;
    processor_process->state.stopped = false;
    process_list = processor_process;
}

// Creates a new process
int create_process(char *name, int (*entry_point)(void)) {
    // Allocate memory for the new process
    struct process *new_process = malloc(sizeof(struct process));
    new_process->name = name;
    new_process->pid = getpid();
    new_process->state.running = true;
    new_process->state.sleeping = false;
    new_process->state.stopped = false;

    // Add the new process to the list of processes
    new_process->next = process_list;
    process_list = new_process;

    // Start the new process
    entry_point();

    return new_process->pid;
}

// Waits for a process to finish
int wait_for_process(int pid) {
    // Find the process with the given PID
    struct process *process = process_list;
    while (process != NULL && process->pid != pid) {
        process = process->next;
    }

    // If the process was found, wait for it to finish
    if (process != NULL) {
        // Wait for the process to stop
        while (process->state.running || process->state.sleeping) {
            yield();
        }

        // Free the process structure
        free(process);

        return 0;
    } else {
        // If the process was not found, return an error
        return -1;
    }
}

// Yields control to the next process
void yield() {
    // Find the current process
    struct process *current_process = process_list;

    // If there are no other processes, just return
    if (current_process == NULL) {
        return;
    }

    // Find the next process
    struct process *next_process = current_process->next;

    // Switch to the next process
    current_process->state.running = false;
    next_process->state.running = true;

    // Update the current process pointer
    process_list = next_process;
}

// Terminates a process
void terminate_process(int pid) {
    // Find the process with the given PID
    struct process *process = process_list;
    while (process != NULL && process->pid != pid) {
        process = process->next;
    }

    // If the process was found,
