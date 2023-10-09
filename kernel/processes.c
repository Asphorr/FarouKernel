#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Define a maximum number of processes
#define MAX_PROCESSES 64

// Structure to represent a process
typedef struct process {
    char* name;         // Name of the process
    int pid;            // Unique identifier for each process
    bool running;       // Whether or not the process is currently running
    bool sleeping;      // Whether or not the process is currently sleeping
    bool stopped;       // Whether or not the process has been terminated
    void* stack_pointer;   // Pointer to the top of the process's stack
    struct process* next;  // Pointer to the next process in the linked list
} process;

// Global variable to store the head of the linked list of processes
static process* process_head = NULL;

// Function to initialize the process manager
void init_process_manager() {
    // Create two initial processes: one for the idle task and one for the processor
    process* idle_task = malloc(sizeof(process));
    strcpy(idle_task->name, "Idle Task");
    idle_task->pid = 0;
    idle_task->running = true;
    idle_task->sleeping = false;
    idle_task->stack_pointer = NULL;
    idle_task->next = NULL;
    
    process* processor = malloc(sizeof(process));
    strcpy(processor->name, "Processor");
    processor->pid = 1;
    processor->running = true;
    processor->sleeping = false;
    processor->stack_pointer = NULL;
    processor->next = NULL;
    
    // Link the two processes together
    idle_task->next = processor;
    processor->next = idle_task;
    
    // Store the head of the linked list
    process_head = idle_task;
}

// Function to create a new process
int create_process(const char* name, void(*entry_point)(void)) {
    // Check if we have reached the maximum number of processes
    if (process_head->next == NULL) {
        printf("Maximum number of processes reached!\n");
        return -1;
    }
    
    // Allocate memory for the new process
    process* new_process = malloc(sizeof(process));
    memset(new_process, 0, sizeof(process));
    
    // Copy the name into the new process
    size_t len = strlen(name);
    new_process->name = malloc(len + 1);
    strncpy(new_process->name, name, len);
    new_process->name[len] = '\0';
    
    // Generate a unique ID for the new process
    static unsigned long id = 2;
    new_process->pid = id++;
    
    // Set the initial values for the new process
    new_process->running = true;
    new_process->sleeping = false;
    new_process->stopped = false;
    new_process->stack_pointer = NULL;
    
    // Insert the new process at the end of the linked list
    process* curr = process_head;
    while (curr->next != NULL) {
        curr = curr->next;
    }
    curr->next = new_process;
    new_process->next = process_head;
    
    // Return the ID of the new process
    return new_process->pid;
}

// Function to wait for a specific process to complete
int wait_for_process(unsigned long pid) {
    // Search for the specified process in the linked list
    process* proc = process_head;
    while (proc != NULL && proc->pid != pid) {
        proc = proc->next;
    }
    
    // If the process was found, wait for it to complete
    if (proc != NULL) {
        // Wait until the process stops running
        while (proc->running || proc->sleeping) {
            yield();
        }
        
        // Free the resources used by the process
        free(proc->name);
        free(proc);
        
        return 0;
    } else {
        // If the process was not found, return an error
        return -1;
    }
}

// Function to terminate a specific process
int terminate_process(unsigned long pid) {
    // Search for the specified process in the linked list
    process* proc = process_head;
    while (proc != NULL && proc->pid != pid) {
        proc = proc->next;
    }
    
    // If the process was found, set its status to stopped
    if (proc != NULL) {
        proc->stopped = true;
        return 0;
    } else {
        // If the process was not found, return an error
        return -1;
    }
}

// Function to yield control back to the operating system
void yield() {
    // Get the current process from the linked list
    process* curr = process_head;
    
    // If the current process is still running, move on to the next process
    if (curr->running) {
        curr = curr->next;
    }
    
    // Loop through all processes until we find one that is ready to run
    while (!curr->running && !curr->stopped) {
        curr = curr->next;
    }
    
    // If we reach the beginning of the linked list again, wrap around to the start
    if (curr == process_head) {
        curr = process_head->next;
    }
    
    // Make the selected process the current process
    process_head = curr;
}

// Example usage of the functions above
int main() {
    // Initialize the process manager
    init_process_manager();
    
    // Create three new processes
    int pids[] = {create_process("Process 1", &my_function),
                  create_process("Process 2", &my_other_function),
                  create_process("Process 3", &yet_another_function)};
    
    // Wait for each process to complete
    for (size_t i = 0; i < sizeof(pids)/sizeof(pids[0]); ++i) {
        wait_for_process(pids[i]);
    }
    
    // Terminate the remaining processes
    for (process* proc = process_head; proc != NULL; proc = proc->next) {
        terminate_process(proc->pid);
    }
    
    return 0;
}
