#include "pgtable.h"

/* Global variables */
struct gdt_entry gdt[GDT_ENTRIES];
struct gdt_ptr gp;
struct tss_entry tss;
struct idt_entry idt[IDT_ENTRIES];
struct idt_ptr idtp;

/* Set a GDT gate */
void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[num].base_low = base & 0xFFFF;
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high = (base >> 24) & 0xFF;

    gdt[num].limit_low = limit & 0xFFFF;
    gdt[num].granularity = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    gdt[num].access = access;
}

/* Set a TSS gate */
void tss_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[num].base_low = base & 0xFFFF;
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high = (base >> 24) & 0xFF;

    gdt[num].limit_low = limit & 0xFFFF;
    gdt[num].granularity = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    gdt[num].access = access;
}

/* Set an IDT gate */
void idt_set_gate(uint8_t num, uint64_t base, uint16_t sel, uint8_t flags) {
    idt[num].base_low = base & 0xFFFF;
    idt[num].base_high = (base >> 16) & 0xFFFF;
    idt[num].selector = sel;
    idt[num].always0 = 0;
    idt[num].flags = flags;
}

/* Setup the GDT */
void setup_gdt() {
    gp.limit = (sizeof(struct gdt_entry) * GDT_ENTRIES) - 1;
    gp.base = (uint64_t)&gdt;

    gdt_set_gate(0, 0, 0, 0, 0);                  /* Null segment */
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);  /* Code segment */
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);  /* Data segment */
    gdt_set_gate(3, (uint32_t)&tss, sizeof(tss) - 1, 0x89, 0x40); /* TSS */

    gdt_flush((uint64_t)&gp);
    tss_flush();
}

/* Setup the IDT */
void setup_idt() {
    idtp.limit = IDT_ENTRIES * sizeof(struct idt_entry) - 1;
    idtp.base = (uint64_t)&idt;

    /* Clear out the entire IDT */
    memset(&idt, 0, sizeof(struct idt_entry) * IDT_ENTRIES);

    /* Add any new ISRs to the IDT here using idt_set_gate */

    idt_flush((uint64_t)&idtp);
}

/* Setup the TSS */
void setup_tss(uint32_t kernel_stack) {
    memset(&tss, 0, sizeof(tss));
    tss.esp0 = kernel_stack;
    tss.ss0 = 0x10;  /* Kernel data segment */

    /* Set TSS gate in GDT */
    tss_set_gate(3, (uint32_t)&tss, sizeof(tss) - 1, 0x89, 0x40);
}
