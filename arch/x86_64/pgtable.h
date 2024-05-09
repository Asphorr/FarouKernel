#ifndef PGTABLE_H
#define PGTABLE_H

#include <stdint.h>
#include <string.h>

#define GDT_ENTRIES 5
#define IDT_ENTRIES 256

/* GDT entry structure */
struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed));

/* TSS (Task State Segment) structure for task switching */
struct tss_entry {
    uint32_t prev_tss;
    uint32_t esp0;
    uint32_t ss0;
    uint32_t esp1;
    uint32_t ss1;
    uint32_t esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax, ecx, edx, ebx;
    uint32_t esp, ebp, esi, edi;
    uint32_t es, cs, ss, ds, fs, gs;
    uint32_t ldt;
    uint16_t trap;
    uint16_t iomap_base;
} __attribute__((packed));

/* IDT entry structure */
struct idt_entry {
    uint16_t base_low;
    uint16_t selector;
    uint8_t always0;
    uint8_t flags;
    uint16_t base_high;
} __attribute__((packed));

/* GDT pointer structure */
struct gdt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

/* IDT pointer structure */
struct idt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

/* Global variable declarations */
extern struct gdt_entry gdt[GDT_ENTRIES];
extern struct gdt_ptr gp;
extern struct tss_entry tss;
extern struct idt_entry idt[IDT_ENTRIES];
extern struct idt_ptr idtp;

/* Assembly functions */
extern void gdt_flush(uint64_t);
extern void idt_flush(uint64_t);
extern void tss_flush();

/* Function declarations */
void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran);
void tss_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran);
void idt_set_gate(uint8_t num, uint64_t base, uint16_t sel, uint8_t flags);
void setup_gdt();
void setup_idt();
void setup_tss(uint32_t kernel_stack);

#endif /* PGTABLE_H */
