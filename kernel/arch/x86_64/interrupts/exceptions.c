#include "exceptions.h"
#include "../drivers/screen.h" // Предполагается, что у вас есть функция для вывода на экран

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
    "Reserved",
    "Reserved",
    "Reserved",
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

void exception_handler(interrupt_frame *frame) {
    if (frame->int_no < 32) {
        print("Exception: ");
        print(exception_messages[frame->int_no]);
        print("\n");
        print("System Halted!\n");
        for (;;);
    }
}

void init_exception_handlers(void) {
    for (int i = 0; i < 32; i++) {
        // Устанавливаем обработчик для каждого исключения
        idt_set_gate(i, (uint64_t)exception_handler, 0x08, 0x8E);
    }
}
