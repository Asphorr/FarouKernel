#ifndef IRQ_H
#define IRQ_H

#include <stdint.h>
#include "../cpu/isr.h"

#define IRQ0 32
#define IRQ1 33
#define IRQ2 34
#define IRQ3 35
#define IRQ4 36
#define IRQ5 37
#define IRQ6 38
#define IRQ7 39
#define IRQ8 40
#define IRQ9 41
#define IRQ10 42
#define IRQ11 43
#define IRQ12 44
#define IRQ13 45
#define IRQ14 46
#define IRQ15 47

void irq_init(void);
void irq_install_handler(int irq, void (*handler)(interrupt_frame *r));
void irq_uninstall_handler(int irq);

#endif
