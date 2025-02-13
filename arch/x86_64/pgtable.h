// pgtable.h
#pragma once
#include <stdint.h>
#include <stddef.h>

#define GDT_ENTRIES 5
#define IDT_ENTRIES 256
#define TSS_IOB_SIZE 0

#pragma pack(push, 1)

typedef struct {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_mid;
    uint8_t access;
    uint8_t limit_high : 4;
    uint8_t flags : 4;
    uint8_t base_high;
} gdt_entry;

typedef struct {
    uint32_t reserved;
    uint64_t rsp[3];
    uint64_t ist[7];
    uint32_t iomap_base;
} tss_entry;

typedef struct {
    uint16_t limit;
    uint64_t base;
} gdt_ptr;

typedef struct {
    uint16_t base_low;
    uint16_t cs;
    uint8_t ist : 3;
    uint8_t reserved0 : 5;
    uint8_t attributes;
    uint16_t base_mid;
    uint32_t base_high;
    uint32_t reserved1;
} idt_entry;

typedef struct {
    uint16_t limit;
    uint64_t base;
} idt_ptr;

#pragma pack(pop)

void gdt_init(void);
void idt_init(void);
void tss_init(uintptr_t stack_top);
