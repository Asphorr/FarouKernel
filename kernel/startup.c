#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <asm/bootparam.h>
#include <asm/elf.h>
#include <asm/page.h>

#define STACK_SIZE 0x1000
#define HEAP_SIZE 0x1000

// Declare the boot parameters
struct boot_params {
    uint32_t magic;
    uint32_t size;
    uint32_t load_addr;
    uint32_t entry_point;
};

// Load the kernel from disk
void load_kernel(const char* filename) {
    // Open the file and read its contents into memory
    int fd = open(filename, O_RDONLY);
    char* buffer = mmap(NULL, 0, 0, PROT_READ, MAP_FILE | MAP_PRIVATE, fd, 0);
    close(fd);

    // Get the ELF header
    Elf32_Ehdr* ehdr = (Elf32_Ehdr*)buffer;

    // Check the ELF header's magic number
    if (memcmp(ehdr->e_ident, "\177ELF\177", 4) != 0) {
        printf("Invalid ELF header\n");
        exit(-1);
    }

    // Get the program headers
    Elf32_Phdr* phdr = (Elf32_Phdr*)((char*)ehdr + ehdr->e_phoff);

    // Loop through the program headers and find the first one with the PT_LOAD flag set
    for (int i = 0; i < ehdr->e_phnum; i++) {
        if (phdr[i].p_type == PT_LOAD) {
            // Calculate the virtual address of the kernel's start address
            uint32_t va = phdr[i].p_vaddr;

            // Map the kernel into memory at the calculated virtual address
            mmap((void*)va, phdr[i].p_filesz, PROT_EXEC, MAP_FIXED | MAP_PRIVATE, -1, 0);

            // Update the entry point to point to the kernel's start address
            struct boot_params params;
            params.magic = BOOT_MAGIC;
            params.size = sizeof(params);
            params.load_addr = va;
            params.entry_point = phdr[i].p_vaddr;

            // Save the boot parameters to the end of the kernel's data segment
            mmap((void*)(va + phdr[i].p_filesz), sizeof(params), PROT_READ | PROT_WRITE, MAP_FIXED | MAP_PRIVATE, -1, 0);

            break;
        }
    }

    // Unmap the ELF header and program headers
    munmap(buffer, ehdr->e_phoffs);
}

// Set up the stack and heap
void setup_memory() {
    // Allocate memory for the stack and heap
    void* stack = malloc(STACK_SIZE);
    void* heap = malloc(HEAP_SIZE);

    // Set up the stack
    ((struct boot_params*)stack)->magic = BOOT_MAGIC;
    ((struct boot_params*)stack)->size = sizeof(struct boot_params);
    ((struct boot_params*)stack)->load_addr = (uint32_t)stack;
    ((struct boot_params*)stack)->entry_point = (uint32_t)start_address;

    // Set up the heap
    heap += HEAP_SIZE;

    // Set the current process's stack and heap pointers
current_process.stack = stack;
current_process.heap = heap;

// Initialize the kernel's memory management functions
mm_init();

// Create a new page directory and set it as the current process's page directory
pd = pd_create();
if (!pd) {
    printk(KERN_ERR "Failed to create page directory\n");
    while (1);
}
current_process.page_dir = pd;

// Set up the page tables for the kernel's text and data segments
pt = pt_create(kernel_text_start, kernel_text_end, PAGE_FLAGS_READONLY);
if (!pt) {
    printk(KERN_ERR "Failed to create page table for kernel text\n");
    while (1);
}
pt_set(pd, KERNEL_TEXT_START, pt);

pt = pt_create(kernel_data_start, kernel_data_end, PAGE_FLAGS_READWRITE);
if (!pt) {
    printk(KERN_ERR "Failed to create page table for kernel data\n");
    while (1);
}
pt_set(pd, KERNEL_DATA_START, pt);

// Set up the page tables for the kernel's stack and heap
pt = pt_create(stack, stack + STACK_SIZE, PAGE_FLAGS_READWRITE);
if (!pt) {
    printk(KERN_ERR "Failed to create page table for kernel stack\n");
    while (1);
}
pt_set(pd, STACK_START, pt);

pt = pt_create(heap, heap + HEAP_SIZE, PAGE_FLAGS_READWRITE);
if (!pt) {
    printk(KERN_ERR "Failed to create page table for kernel heap\n");
    while (1);
}
pt_set(pd, HEAP_START, pt);

// Enable paging and set the page fault handler
page_fault_handler = kmem_alloc(sizeof(page_fault_handler_t));
if (!page_fault_handler) {
    printk(KERN_ERR "Failed to allocate memory for page fault handler\n");
    while (1);
}
page_fault_handler->handler = &page_fault_handler_function;
page_fault_handler->next = NULL;

set_intr_gate(PAGE_FAULT_VECTOR, page_fault_handler);
enable_paging();

// Start the kernel
printk(KERN_INFO "Starting kernel...\n");

while (1) {
    // Handle interrupts
    intr_handle();

    // Run the next process
    run_next_process();
}
