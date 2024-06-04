#include "kernel.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

// External assembly functions
extern void gdt_flush();
extern void idt_flush();
extern void setup_gdt();
extern void setup_idt();
extern void setup_tss();
extern void tss_flush();
extern void isr_install();

// Stack pointer for TSS Ring 0
extern uint8_t _stack_start[];

// Kernel entry point
void kernel_init() {
    // Initialize GDT and TSS
    setup_gdt();
    setup_tss();
    tss_flush();

    // Initialize IDT and ISRs
    setup_idt();
    isr_install();
    
    // Additional architecture-specific initialization
    printf("Kernel initialized for x86_64 architecture.\n");
}

// Example ISR handler function
void irq_handler(uint64_t *stack_frame) {
    uint64_t isr_number = stack_frame[1]; // ISR number is the second pushed value

    switch (isr_number) {
        case 0:
            printf("ISR0: Divide by Zero Exception\n");
            break;
        case 14:
            printf("ISR14: Page Fault Exception\n");
            break;
        default:
            printf("ISR%lu: Unhandled Exception\n", isr_number);
            break;
    }

    // Clear the interrupt flag and return
}
