#include "isr.h"
#include <stdio.h>

extern void isr_common_stub(void);

static idt_entry_t idt[IDT_ENTRIES];
static idt_ptr_t idt_ptr;

static const char *exception_messages[] = {
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Into Detected Overflow",
    "Out of Bounds",
    "Invalid Opcode",
    "No Coprocessor",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Bad TSS",
    "Segment Not Present",
    "Stack Fault",
    "General Protection Fault",
    "Page Fault",
    "Unknown Interrupt",
    "Coprocessor Fault",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point Exception",
    "Virtualization Exception",
    "Control Protection Exception",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved"
};

static void idt_set_gate(uint8_t num, uint64_t base, uint16_t sel, uint8_t flags) {
    idt[num].base_low = base & 0xFFFF;
    idt[num].base_mid = (base >> 16) & 0xFFFF;
    idt[num].base_high = (base >> 32) & 0xFFFFFFFF;
    idt[num].sel = sel;
    idt[num].always0 = 0;
    idt[num].flags = flags;
    idt[num].reserved = 0;
}

void idt_init(void) {
    idt_ptr.limit = sizeof(idt) - 1;
    idt_ptr.base = (uint64_t)&idt;

    // Clear out the entire IDT, initializing it to zeros
    for (int i = 0; i < IDT_ENTRIES; i++) {
        idt_set_gate(i, 0, 0, 0);
    }

    // Set up ISRs for the first 32 interrupts (exceptions)
    for (int i = 0; i < 32; i++) {
        idt_set_gate(i, (uint64_t)isr_common_stub, KERNEL_CODE_SEGMENT, IDT_FLAG_INTERRUPT_GATE);
    }

    // Load the IDT
    __asm__ volatile ("lidt %0" : : "m"(idt_ptr));
}

void isr_handler(uint64_t isr_number) {
    if (isr_number < 32) {
        printf("Received interrupt: %s\n", exception_messages[isr_number]);
    } else {
        printf("Received interrupt: %llu\n", isr_number);
    }
}
