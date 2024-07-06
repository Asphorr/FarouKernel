#include "irq.h"
#include "pic.h"
#include "../cpu/idt.h"

void *irq_routines[16] = { 0 };

void irq_install_handler(int irq, void (*handler)(interrupt_frame *r)) {
    irq_routines[irq] = handler;
}

void irq_uninstall_handler(int irq) {
    irq_routines[irq] = 0;
}

void irq_handler(interrupt_frame *frame) {
    void (*handler)(interrupt_frame *frame);

    handler = irq_routines[frame->int_no - 32];
    if (handler) {
        handler(frame);
    }

    // Отправляем сигнал EOI (End of Interrupt) контроллеру PIC
    if (frame->int_no >= 40) {
        pic_send_eoi(PIC2_COMMAND);
    }
    pic_send_eoi(PIC1_COMMAND);
}

void irq_init(void) {
    pic_remap(IRQ0, IRQ8);

    for (int i = 0; i < 16; i++) {
        idt_set_gate(32 + i, (uint64_t)irq_handler, 0x08, 0x8E);
    }
}
