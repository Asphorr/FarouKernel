#include <iostream>
#include <string>
#include <vector>

using namespace std;

constexpr auto KERNEL_ID = 0x12345678;

// Structure to represent a process
struct Process {
    uint32_t pid;
    string name;
    vector<uint32_t> stack;
    vector<uint32_t> heap;
};

// Global variables
Process current_process;
uint32_t kernel_stack_size = 4096;
uint32_t kernel_heap_size = 16384;

// Function to create a new process
void create_process(const string& name, const void* entry) {
    // Allocate memory for the process's stack and heap
    vector<uint32_t> stack(kernel_stack_size);
    vector<uint32_t> heap(kernel_heap_size);

    // Create a new process structure
    Process* p = new Process{};
    p->pid = get_free_pid();
    p->name = name;
    p->stack = move(stack);
    p->heap = move(heap);

    // Set up the process's stack frame
    uint32_t* esp = p->stack.data() + kernel_stack_size - sizeof(uint32_t);
    *reinterpret_cast<uint32_t*>(esp) = p->pid;
    esp -= sizeof(uint32_t);
    *reinterpret_cast<uint32_t*>(esp) = 0; // return address
    esp -= sizeof(uint32_t);
    *reinterpret_cast<uint32_t*>(esp) = 0; // arg1
    esp -= sizeof(uint32_t);
    *reinterpret_cast<uint32_t*>(esp) = 0; // arg2

    // Set up the process's heap
    p->heap = move(heap);

    // Set the current process to the new process
    current_process = p;

    // Start the new process
    (*entry)(p);
}

// Function to handle a system call
int handle_syscall(uint32_t num, uint32_t arg1, uint32_t arg2) {
    switch (num) {
        case SYSCALL_CREATE_PROCESS:
            create_process("my_program", (void*)arg1);
            break;
        default:
            cout << "Unsupported system call " << num << endl;
            return -ENOSYS;
    }
    return 0;
}

// Idle thread function
void idletask() {
    while (true) {
        // Handle system calls
        uint32_t num;
        uint32_t arg1;
        uint32_t arg2;
        if (get_syscall(&num, &arg1, &arg2)) {
            int ret = handle_syscall(num, arg1, arg2);
            if (ret != 0) {
                cout << "Error in system call " << ret << endl;
            }
        }

        // Do nothing else
        pause();
    }
}

// Initialize the kernel
void kernel_init() {
    // Set up the stack and heap sizes
    kernel_stack_size = 4096;
    kernel_heap_size = 16384;

    // Create the first process
    create_process("idle", (void*)idletask);

    // Set up the idle thread
    current_process = &idle_process;
    idle_process.pid = 0;
    idle_process.name = "idle";
    idle_process.stack = (void*)&kernel_stack[kernel_stack_size];
    idle_process.heap = (void*)&kernel_heap[kernel_heap_size];

    // Start the idle thread
    idletask();
}

// Entry point for the kernel
void kernel_entry() {
    kernel_init();
}
