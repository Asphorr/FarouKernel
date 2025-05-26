// pgtable.c
#include "pgtable.h"
#include <string.h>

static gdt_entry gdt[GDT_ENTRIES];
static tss_entry tss;
static idt_entry idt[IDT_ENTRIES];

static void gdt_load(const gdt_ptr* ptr) {
    __asm__ volatile("lgdt %0" : : "m"(*ptr));
}

static void tss_load(uint16_t sel) {
    __asm__ volatile("ltr %0" : : "r"(sel));
}

static void idt_load(const idt_ptr* ptr) {
    __asm__ volatile("lidt %0" : : "m"(*ptr));
}

void gdt_init(void) {
    gdt_ptr gp = {
        .limit = sizeof(gdt) - 1,
        .base = (uintptr_t)&gdt
    };

    // Null descriptor
    gdt[0] = (gdt_entry){0};

    // Kernel code (64-bit)
    gdt[1] = (gdt_entry){
        .access = 0x9A,
        .flags = 0xA
    };

    // Kernel data
    gdt[2] = (gdt_entry){
        .access = 0x92
    };

    // User code
    gdt[3] = (gdt_entry){
        .access = 0xFA,
        .flags = 0xA
    };

    // User data
    gdt[4] = (gdt_entry){
        .access = 0xF2
    };

    gdt_load(&gp);
}

void idt_init(void) {
    idt_ptr idtp = {
        .limit = sizeof(idt) - 1,
        .base = (uintptr_t)&idt
    };

    memset(idt, 0, sizeof(idt));
    idt_load(&idtp);
}

void tss_init(uintptr_t stack_top) {
    memset(&tss, 0, sizeof(tss));
    tss.rsp[0] = stack_top;
    tss.iomap_base = sizeof(tss);

    gdt[GDT_ENTRIES-1] = (gdt_entry){
        .limit_low = sizeof(tss),
        .base_low = (uintptr_t)&tss,
        .base_mid = ((uintptr_t)&tss >> 16),
        .access = 0x89,
        .limit_high = 0,
        .flags = 0,
        .base_high = ((uintptr_t)&tss >> 24)
    };

    tss_load((GDT_ENTRIES-1) << 3);
}
