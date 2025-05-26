#ifndef KERNEL_H
#define KERNEL_H

#include <stdint.h>

// Function declarations
void gdt_flush();
void idt_flush();
void setup_gdt();
void setup_idt();
void setup_tss();
void tss_flush();
void isr_install();

// Kernel initialization
void kernel_init();

// ISR handler declaration
void irq_handler(uint64_t *stack_frame);

#endif // KERNEL_H
