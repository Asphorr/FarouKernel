#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <asm/io.h>
#include <asm/sigcontext.h>
#include <asm/system.h>

// Define the IDT entries
const int num_ids = 256;
idt_entry_t idts[num_ids];

// Define the GDT entries
const int num_gds = 256;
gdt_entry_t gdts[num_gds];

// Define the interrupt handlers
void (*handlers[])(struct interrupt_frame *) = {
    [0x20] = &custom_handler, // Vector 0x20: Custom handler
    [0x21] = &divide_by_zero_handler, // Vector 0x21: Divide by zero
    [0x22] = &debug_handler, // Vector 0x22: Debug
};

// Initialize the IDT and GDT
void init_idt_gdt() {
    // Initialize the IDT with the default entry
    for (int i = 0; i < num_ids; i++) {
        idts[i].offset = (uintptr_t)default_handler;
        idts[i].selector = KERNEL_CS;
        idts[i].ists = 0;
        idts[i].type = 0x0A; // Interrupt gate, 32-bit
    }

    // Initialize the GDT with the default entry
    for (int i = 0; i < num_gds; i++) {
        gdts[i].address = (uintptr_t)default_handler;
        gdts[i].limit = 0xFFFFF;
        gdts[i].segments = 0x0A; // Code segment, read-only
    }
}

// Load the IDT and GDT
void load_idt_gdt() {
    // Load the IDT
    lidt(idts);

    // Load the GDT
    lgdt(gdts);
}

// Custom interrupt handler
void custom_handler(struct interrupt_frame *frame) {
    printf("Custom interrupt handler called\n");

    // Handle the interrupt here
    return;
}

// Divide by zero handler
void divide_by_zero_handler(struct interrupt_frame *frame) {
    printf("Divide by zero handler called\n");

    // Handle the divide by zero fault here
    return;
}

// Debug handler
void debug_handler(struct interrupt_frame *frame) {
    printf("Debug handler called\n");

    // Handle the debug event here
    return;
}

// Default interrupt handler
void default_handler(struct interrupt_frame *frame) {
    printf("Default interrupt handler called\n");

    // Handle the interrupt here
    return;
}

int main() {
    // Initialize the IDT and GDT
    init_idt_gdt();

    // Load the IDT and GDT
    load_idt_gdt();

    // Enable interrupts in the CPU
    sti();

    // Perform some operations that may trigger interrupts
    printf("Before fork()\n");
    pid_t pid = fork();
    if (pid == 0) {
        // Child process
        printf("Child process\n");
    } else {
        // Parent process
        printf("Parent process\n");
    }

    // Wait for the child process to finish
    wait(NULL);

    // Disable interrupts in the CPU
    cli();

    return 0;
}
