#include <stdint.h>
#include <io.h>

/* PIC Ports */
#define PIC1_COMMAND 0x20
#define PIC1_DATA 0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA 0xA1

/* PIC Commands */
#define PIC_EOI 0x20

/* Initialization Control Words (ICW) */
#define ICW1_INIT 0x10
#define ICW1_ICW4 0x01
#define ICW4_8086 0x01

/* Remap IRQs 0-15 to IDT entries 32-47 */
void remap_pic() {
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    outb(PIC1_DATA, 0x20);  /* Master PIC vector offset */
    outb(PIC2_DATA, 0x28);  /* Slave PIC vector offset */
    outb(PIC1_DATA, 0x04);  /* Tell Master PIC about Slave at IRQ2 */
    outb(PIC2_DATA, 0x02);  /* Tell Slave PIC its cascade identity */
    outb(PIC1_DATA, ICW4_8086);
    outb(PIC2_DATA, ICW4_8086);

    /* Mask all interrupts */
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
}

/* Send End-of-Interrupt (EOI) signal to PICs */
void send_eoi(uint8_t irq) {
    if (irq >= 8) {
        outb(PIC2_COMMAND, PIC_EOI);
    }
    outb(PIC1_COMMAND, PIC_EOI);
}

/* Initialize the PICs and remap IRQs */
void init_pic() {
    remap_pic();
    /* Unmask desired IRQs here (0: Timer, 1: Keyboard, etc.) */
    outb(PIC1_DATA, 0xFC);  /* Unmask IRQ0 and IRQ1 */
    outb(PIC2_DATA, 0xFF);  /* Mask all IRQs on Slave PIC */
}
