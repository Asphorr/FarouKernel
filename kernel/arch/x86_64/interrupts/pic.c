#include "pic.h"
#include "../cpu/ports.h"

void pic_send_eoi(unsigned char irq) {
    if(irq >= 8)
        port_byte_out(PIC2_COMMAND, PIC_EOI);
    port_byte_out(PIC1_COMMAND, PIC_EOI);
}

void pic_remap(int offset1, int offset2) {
    unsigned char a1, a2;

    a1 = port_byte_in(PIC1_DATA);
    a2 = port_byte_in(PIC2_DATA);

    port_byte_out(PIC1_COMMAND, 0x11);
    port_byte_out(PIC2_COMMAND, 0x11);
    port_byte_out(PIC1_DATA, offset1);
    port_byte_out(PIC2_DATA, offset2);
    port_byte_out(PIC1_DATA, 4);
    port_byte_out(PIC2_DATA, 2);
    port_byte_out(PIC1_DATA, 0x01);
    port_byte_out(PIC2_DATA, 0x01);

    port_byte_out(PIC1_DATA, a1);
    port_byte_out(PIC2_DATA, a2);
}

void pic_set_mask(unsigned char IRQline) {
    uint16_t port;
    uint8_t value;

    if(IRQline < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        IRQline -= 8;
    }
    value = port_byte_in(port) | (1 << IRQline);
    port_byte_out(port, value);
}

void pic_clear_mask(unsigned char IRQline) {
    uint16_t port;
    uint8_t value;

    if(IRQline < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        IRQline -= 8;
    }
    value = port_byte_in(port) & ~(1 << IRQline);
    port_byte_out(port, value);
}
