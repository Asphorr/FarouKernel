#include <asm/desc.h>

#define GDT_ENTRIES 3
#define IDT_ENTRIES 256

struct gdt_entry {
    unsigned short limit_low;
    unsigned short base_low;
    unsigned char base_middle;
    unsigned char access;
    unsigned char granularity;
    unsigned char base_high;
} __attribute__((packed));

struct idt_entry {
    unsigned short base_low;
    unsigned short selector;
    unsigned char always0;
    unsigned char flags;
    unsigned short base_high;
} __attribute__((packed));

struct gdt_ptr {
    unsigned short limit;
    unsigned int base;
} __attribute__((packed));

struct idt_ptr {
    unsigned short limit;
    unsigned int base;
} __attribute__((packed));

struct gdt_entry gdt[GDT_ENTRIES];
struct gdt_ptr gp;

struct idt_entry idt[IDT_ENTRIES];
struct idt_ptr idtp;

extern void gdt_flush();
extern void idt_flush();

void gdt_set_gate(int num, unsigned long base, unsigned long limit, unsigned char access, unsigned char gran)
{
    gdt[num].base_low = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high = (base >> 24) & 0xFF;

    gdt[num].limit_low = (limit & 0xFFFF);
    gdt[num].granularity = ((limit >> 16) & 0x0F);

    gdt[num].granularity |= (gran & 0xF0);
    gdt[num].access = access;
}

void idt_set_gate(unsigned char num, unsigned long base, unsigned short sel, unsigned char flags)
{
    idt[num].base_low = base & 0xFFFF;
    idt[num].base_high = (base >> 16) & 0xFFFF;

    idt[num].selector = sel;
    idt[num].always0 = 0;
    idt[num].flags = flags;
}

void setup_gdt()
{
    gp.limit = (sizeof(struct gdt_entry) * GDT_ENTRIES) - 1;
    gp.base = (unsigned int)&gdt;

    gdt_set_gate(0, 0, 0, 0, 0);                    /* Null segment */
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);    /* Code segment */
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);    /* Data segment */

    gdt_flush();
}

void setup_idt()
{
    idtp.limit = IDT_ENTRIES * sizeof(struct idt_entry) - 1;
    idtp.base = (unsigned int)&idt;

    /* Clear out the entire IDT, initializing it to zeros */
    memset(&idt, 0, sizeof(struct idt_entry) * IDT_ENTRIES);

    /* Add any new ISRs to the IDT here using idt_set_gate */

    idt_flush();
}
