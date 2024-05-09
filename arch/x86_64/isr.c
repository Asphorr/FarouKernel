// isr.c
#include "pgtable.h"
#include <stdint.h>
#include <stdio.h>

extern void isr0();
extern void isr1();
extern void isr2();
extern void isr3();
extern void isr4();
extern void isr5();
extern void isr6();
extern void isr7();
extern void isr8();
extern void isr9();
extern void isr10();
extern void isr11();
extern void isr12();
extern void isr13();
extern void isr14();
extern void isr15();
extern void isr16();
extern void isr17();
extern void isr18();
extern void isr19();
extern void isr20();
extern void isr21();
extern void isr22();
extern void isr23();
extern void isr24();
extern void isr25();
extern void isr26();
extern void isr27();
extern void isr28();
extern void isr29();
extern void isr30();
extern void isr31();

void idt_set_gate(uint8_t num, uint64_t base, uint16_t sel, uint8_t flags);

/* Exception messages */
const char *exception_messages[] = {
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
    "Reserved"
};

/* Setup ISRs */
void setup_isrs() {
    idt_set_gate(0, (uint64_t)isr0, 0x08, 0x8E);
    idt_set_gate(1, (uint64_t)isr1, 0x08, 0x8E);
    idt_set_gate(2, (uint64_t)isr2, 0x08, 0x8E);
    idt_set_gate(3, (uint64_t)isr3, 0x08, 0x8E);
    idt_set_gate(4, (uint64_t)isr4, 0x08, 0x8E);
    idt_set_gate(5, (uint64_t)isr5, 0x08, 0x8E);
    idt_set_gate(6, (uint64_t)isr6, 0x08, 0x8E);
    idt_set_gate(7, (uint64_t)isr7, 0x08, 0x8E);
    idt_set_gate(8, (uint64_t)isr8, 0x08, 0x8E);
    idt_set_gate(9, (uint64_t)isr9, 0x08, 0x8E);
    idt_set_gate(10, (uint64_t)isr10, 0x08, 0x8E);
    idt_set_gate(11, (uint64_t)isr11, 0x08, 0x8E);
    idt_set_gate(12, (uint64_t)isr12, 0x08, 0x8E);
    idt_set_gate(13, (uint64_t)isr13, 0x08, 0x8E);
    idt_set_gate(14, (uint64_t)isr14, 0x08, 0x8E);
    idt_set_gate(15, (uint64_t)isr15, 0x08, 0x8E);
    idt_set_gate(16, (uint64_t)isr16, 0x08, 0x8E);
    idt_set_gate(17, (uint64_t)isr17, 0x08, 0x8E);
    idt_set_gate(18, (uint64_t)isr18, 0x08, 0x8E);
    idt_set_gate(19, (uint64_t)isr19, 0x08, 0x8E);
    idt_set_gate(20, (uint64_t)isr20, 0x08, 0x8E);
    idt_set_gate(21, (uint64_t)isr21, 0x08, 0x8E);
    idt_set_gate(22, (uint64_t)isr22, 0x08, 0x8E);
    idt_set_gate(23, (uint64_t)isr23, 0x08, 0x8E);
    idt_set_gate(24, (uint64_t)isr24, 0x08, 0x8E);
    idt_set_gate(25, (uint64_t)isr25, 0x08, 0x8E);
    idt_set_gate(26, (uint64_t)isr26, 0x08, 0x8E);
    idt_set_gate(27, (uint64_t)isr27, 0x08, 0x8E);
    idt_set_gate(28, (uint64_t)isr28, 0x08, 0x8E);
    idt_set_gate(29, (uint64_t)isr29, 0x08, 0x8E);
    idt_set_gate(30, (uint64_t)isr30, 0x08, 0x8E);
    idt_set_gate(31, (uint64_t)isr31, 0x08, 0x8E);
}

/* Function to set an entry in the IDT */
void idt_set_gate(uint8_t num, uint64_t base, uint16_t sel, uint8_t flags) {
    extern struct idt_entry_t idt[];

    idt[num].base_low = base & 0xFFFF;
    idt[num].base_mid = (base >> 16) & 0xFFFF;
    idt[num].base_high = (base >> 32) & 0xFFFFFFFF;
    idt[num].sel = sel;
    idt[num].always0 = 0;
    idt[num].flags = flags | 0x60; // Set DPL to 3 for user-mode access if needed
}

/* IDT entry structure for 64-bit mode */
struct idt_entry_t {
    uint16_t base_low;
    uint16_t sel;
    uint8_t always0;
    uint8_t flags;
    uint16_t base_mid;
    uint32_t base_high;
    uint32_t reserved;
};

/* IDT pointer structure */
struct idt_ptr_t {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

/* Declare an IDT of 256 entries */
struct idt_entry_t idt[256];
struct idt_ptr_t idt_ptr;

/* Load the IDT using the lidt instruction */
extern void idt_load();

/* Initialize the IDT */
void idt_init() {
    idt_ptr.limit = sizeof(idt) - 1;
    idt_ptr.base = (uint64_t)&idt;

    /* Clear out the entire IDT, initializing it to zero */
    for (int i = 0; i < 256; ++i) {
        idt[i].base_low = 0;
        idt[i].base_mid = 0;
        idt[i].base_high = 0;
        idt[i].sel = 0;
        idt[i].always0 = 0;
        idt[i].flags = 0;
        idt[i].reserved = 0;
    }

    /* Setup ISRs for the first 32 IRQs */
    setup_isrs();

    /* Load the IDT */
    idt_load();
}

/* Assembly code to load the IDT */
__asm__ (
    ".global idt_load       \n"
    "idt_load:              \n"
    "    lea idt_ptr(%rip), %rdi \n"
    "    lidt (%rdi)        \n"
    "    ret                \n"
);

/* Assembly code for ISRs (example for `isr0`) */
__asm__ (
    ".global isr0           \n"
    "isr0:                  \n"
    "    cli                \n"
    "    pushq $0           \n" /* Error code placeholder */
    "    pushq $0           \n" /* Interrupt number */
    "    pushq %rdi         \n" /* General purpose registers */
    "    pushq %rsi         \n"
    "    pushq %rbp         \n"
    "    pushq %rsp         \n"
    "    pushq %rbx         \n"
    "    pushq %rdx         \n"
    "    pushq %rcx         \n"
    "    pushq %rax         \n"
    "    movq %gs, %rax     \n"
    "    pushq %rax         \n"
    "    movq %fs, %rax     \n"
    "    pushq %rax         \n"
    "    movq %es, %rax     \n"
    "    pushq %rax         \n"
    "    movq %ds, %rax     \n"
    "    pushq %rax         \n"
    "    call isr_handler   \n"
    "    popq %rax          \n" /* Restore segment registers */
    "    movq %rax, %ds     \n"
    "    popq %rax          \n"
    "    movq %rax, %es     \n"
    "    popq %rax          \n"
    "    movq %rax, %fs     \n"
    "    popq %rax          \n"
    "    movq %rax, %gs     \n"
    "    popq %rax          \n" /* Restore general purpose registers */
    "    popq %rcx          \n"
    "    popq %rdx          \n"
    "    popq %rbx          \n"
    "    popq %rsp          \n"
    "    popq %rbp          \n"
    "    popq %rsi          \n"
    "    popq %rdi          \n"
    "    addq $16, %rsp     \n" /* Remove error code and interrupt number */
    "    sti                \n"
    "    iretq              \n"
);

/* Repeat similar ISR assembly stubs for each ISR (isr1, isr2, ..., isr31) */

/* Example for isr1 */
__asm__ (
    ".global isr1           \n"
    "isr1:                  \n"
    "    cli                \n"
    "    pushq $0           \n"
    "    pushq $1           \n"
    "    pushq %rdi         \n"
    "    pushq %rsi         \n"
    "    pushq %rbp         \n"
    "    pushq %rsp         \n"
    "    pushq %rbx         \n"
    "    pushq %rdx         \n"
    "    pushq %rcx         \n"
    "    pushq %rax         \n"
    "    movq %gs, %rax     \n"
    "    pushq %rax         \n"
    "    movq %fs, %rax     \n"
    "    pushq %rax         \n"
    "    movq %es, %rax     \n"
    "    pushq %rax         \n"
    "    movq %ds, %rax     \n"
    "    pushq %rax         \n"
    "    call isr_handler   \n"
    "    popq %rax          \n"
    "    movq %rax, %ds     \n"
    "    popq %rax          \n"
    "    movq %rax, %es     \n"
    "    popq %rax          \n"
    "    movq %rax, %fs     \n"
    "    popq %rax          \n"
    "    movq %rax, %gs     \n"
    "    popq %rax          \n"
    "    popq %rcx          \n"
    "    popq %rdx          \n"
    "    popq %rbx          \n"
    "    popq %rsp          \n"
    "    popq %rbp          \n"
    "    popq %rsi          \n"
    "    popq %rdi          \n"
    "    addq $16, %rsp     \n"
    "    sti                \n"
    "    iretq              \n"
);
