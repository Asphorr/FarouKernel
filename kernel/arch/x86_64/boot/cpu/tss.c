#include "tss.h"
#include "gdt.h"

struct tss_entry tss;

void tss_init(uint64_t kernel_stack) {
    uint64_t base = (uint64_t)&tss;

    // Initialize TSS
    tss.rsp0 = kernel_stack;
    tss.iopb_offset = sizeof(tss);

    // Add TSS descriptor to GDT
    gdt_set_gate(5, base, sizeof(tss), 0x89, 0x00);

    // Load TSS
    __asm__ volatile ("ltr %%ax" : : "a"(0x28));
}
