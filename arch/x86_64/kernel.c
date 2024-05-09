// kernel.c
#include "pgtable.h"

void kernel_main() {
    setup_gdt();
    setup_idt();
    setup_tss(0x9FC00);  // Example stack pointer

    // Additional kernel initialization code...
    while (1) {
        __asm__ __volatile__("hlt");
    }
}
