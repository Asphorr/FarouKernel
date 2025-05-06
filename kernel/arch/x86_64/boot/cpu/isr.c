#include "isr.h"
#include "idt.h"

extern void isr0();
extern void isr1();
// ... Define more ISRs as needed

void isr_init(void) {
    idt_set_gate(0, (uint64_t)isr0, 0x08, 0x8E);
    idt_set_gate(1, (uint64_t)isr1, 0x08, 0x8E);
    // ... Set up more ISRs
}

void isr_handler(interrupt_frame *frame) {
    // Handle the interrupt
    // You can switch on frame->int_no to handle different interrupts
}
