#include <stdint.h>
#include <string.h>

#define IDT_ENTRIES 256
#define KERNEL_CS 0x08

/* IDT Entry Structure */
typedef struct {
    uint16_t base_low;
    uint16_t sel;
    uint8_t always0;
    uint8_t flags;
    uint16_t base_mid;
    uint32_t base_high;
    uint32_t always1;
} __attribute__((packed)) idt_entry_t;

/* IDT Pointer Structure */
typedef struct {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) idt_ptr_t;

idt_entry_t idt[IDT_ENTRIES];
idt_ptr_t idt_ptr;

/* External ISR Functions */
extern void isr0();
extern void isr1();
extern void isr2();
extern void isr3();
extern void isr4();
extern void isr5();
extern void isr6();
extern void isr7();
extern void isr8();
extern void isr9();
extern void isr10();
extern void isr11();
extern void isr12();
extern void isr13();
extern void isr14();
extern void isr15();
extern void isr16();
extern void isr17();
extern void isr18();
extern void isr19();
extern void isr20();
extern void isr21();
extern void isr22();
extern void isr23();
extern void isr24();
extern void isr25();
extern void isr26();
extern void isr27();
extern void isr28();
extern void isr29();
extern void isr30();
extern void isr31();

/* External IRQ Functions */
extern void isr32();
extern void isr33();
extern void isr34();
extern void isr35();
extern void isr36();
extern void isr37();
extern void isr38();
extern void isr39();
extern void isr40();
extern void isr41();
extern void isr42();
extern void isr43();
extern void isr44();
extern void isr45();
extern void isr46();
extern void isr47();

void set_idt_entry(int num, uint64_t base, uint16_t sel, uint8_t flags) {
    idt[num].base_low = base & 0xFFFF;
    idt[num].sel = sel;
    idt[num].always0 = 0;
    idt[num].flags = flags;
    idt[num].base_mid = (base >> 16) & 0xFFFF;
    idt[num].base_high = (base >> 32) & 0xFFFFFFFF;
    idt[num].always1 = 0;
}

void load_idt() {
    idt_ptr.limit = sizeof(idt) - 1;
    idt_ptr.base = (uint64_t)&idt;
    asm volatile("lidt %0" : : "m"(idt_ptr));
}

void init_idt() {
    memset(&idt, 0, sizeof(idt));

    set_idt_entry(0, (uint64_t)isr0, KERNEL_CS, 0x8E);
    set_idt_entry(1, (uint64_t)isr1, KERNEL_CS, 0x8E);
    set_idt_entry(2, (uint64_t)isr2, KERNEL_CS, 0x8E);
    set_idt_entry(3, (uint64_t)isr3, KERNEL_CS, 0x8E);
    set_idt_entry(4, (uint64_t)isr4, KERNEL_CS, 0x8E);
    set_idt_entry(5, (uint64_t)isr5, KERNEL_CS, 0x8E);
    set_idt_entry(6, (uint64_t)isr6, KERNEL_CS, 0x8E);
    set_idt_entry(7, (uint64_t)isr7, KERNEL_CS, 0x8E);
    set_idt_entry(8, (uint64_t)isr8, KERNEL_CS, 0x8E);
    set_idt_entry(9, (uint64_t)isr9, KERNEL_CS, 0x8E);
    set_idt_entry(10, (uint64_t)isr10, KERNEL_CS, 0x8E);
    set_idt_entry(11, (uint64_t)isr11, KERNEL_CS, 0x8E);
    set_idt_entry(12, (uint64_t)isr12, KERNEL_CS, 0x8E);
    set_idt_entry(13, (uint64_t)isr13, KERNEL_CS, 0x8E);
    set_idt_entry(14, (uint64_t)isr14, KERNEL_CS, 0x8E);
    set_idt_entry(15, (uint64_t)isr15, KERNEL_CS, 0x8E);
    set_idt_entry(16, (uint64_t)isr16, KERNEL_CS, 0x8E);
    set_idt_entry(17, (uint64_t)isr17, KERNEL_CS, 0x8E);
    set_idt_entry(18, (uint64_t)isr18, KERNEL_CS, 0x8E);
    set_idt_entry(19, (uint64_t)isr19, KERNEL_CS, 0x8E);
    set_idt_entry(20, (uint64_t)isr20, KERNEL_CS, 0x8E);
    set_idt_entry(21, (uint64_t)isr21, KERNEL_CS, 0x8E);
    set_idt_entry(22, (uint64_t)isr22, KERNEL_CS, 0x8E);
    set_idt_entry(23, (uint64_t)isr23, KERNEL_CS, 0x8E);
    set_idt_entry(24, (uint64_t)isr24, KERNEL_CS, 0x8E);
    set_idt_entry(25, (uint64_t)isr25, KERNEL_CS, 0x8E);
    set_idt_entry(26, (uint64_t)isr26, KERNEL_CS, 0x8E);
    set_idt_entry(27, (uint64_t)isr27, KERNEL_CS, 0x8E);
    set_idt_entry(28, (uint64_t)isr28, KERNEL_CS, 0x8E);
    set_idt_entry(29, (uint64_t)isr29, KERNEL_CS, 0x8E);
    set_idt_entry(30, (uint64_t)isr30, KERNEL_CS, 0x8E);
    set_idt_entry(31, (uint64_t)isr31, KERNEL_CS, 0x8E);

    set_idt_entry(32, (uint64_t)isr32, KERNEL_CS, 0x8E);
    set_idt_entry(33, (uint64_t)isr33, KERNEL_CS, 0x8E);
    set_idt_entry(34, (uint64_t)isr34, KERNEL_CS, 0x8E);
    set_idt_entry(35, (uint64_t)isr35, KERNEL_CS, 0x8E);
    set_idt_entry(36, (uint64_t)isr36, KERNEL_CS, 0x8E);
    set_idt_entry(37, (uint64_t)isr37, KERNEL_CS, 0x8E);
    set_idt_entry(38, (uint64_t)isr38, KERNEL_CS, 0x8E);
    set_idt_entry(39, (uint64_t)isr39, KERNEL_CS, 0x8E);
    set_idt_entry(40, (uint64_t)isr40, KERNEL_CS, 0x8E);
    set_idt_entry(41, (uint64_t)isr41, KERNEL_CS, 0x8E);
    set_idt_entry(42, (uint64_t)is3, (uint64_t)isr3, KERNEL_CS, 0x8E);
    set_idt_entry(4, (uint64_t)isr4, KERNEL_CS, 0x8E);
    set_idt_entry(5, (uint64_t)isr5, KERNEL_CS, 0x8E);
    set_idt_entry(6, (uint64_t)isr6, KERNEL_CS, 0x8E);
    set_idt_entry(7, (uint64_t)isr7, KERNEL_CS, 0x8E);
    set_idt_entry(8, (uint64_t)isr8, KERNEL_CS, 0x8E);
    set_idt_entry(9, (uint64_t)isr9, KERNEL_CS, 0x8E);
    set_idt_entry(10, (uint64_t)isr10, KERNEL_CS, 0x8E);
    set_idt_entry(11, (uint64_t)isr11, KERNEL_CS, 0x8E);
    set_idt_entry(12, (uint64_t)isr12, KERNEL_CS, 0x8E);
    set_idt_entry(13, (uint64_t)isr13, KERNEL_CS, 0x8E);
    set_idt_entry(14, (uint64_t)isr14, KERNEL_CS, 0x8E);
    set_idt_entry(15, (uint64_t)isr15, KERNEL_CS, 0x8E);
    set_idt_entry(16, (uint64_t)isr16, KERNEL_CS, 0x8E);
    set_idt_entry(17, (uint64_t)isr17, KERNEL_CS, 0x8E);
    set_idt_entry(18, (uint64_t)isr18, KERNEL_CS, 0x8E);
    set_idt_entry(19, (uint64_t)isr19, KERNEL_CS, 0x8E);
    set_idt_entry(20, (uint64_t)isr20, KERNEL_CS, 0x8E);
    set_idt_entry(21, (uint64_t)isr21, KERNEL_CS, 0x8E);
    set_idt_entry(22, (uint64_t)isr22, KERNEL_CS, 0x8E);
    set_idt_entry(23, (uint64_t)isr23, KERNEL_CS, 0x8E);
    set_idt_entry(24, (uint64_t)isr24, KERNEL_CS, 0x8E);
    set_idt_entry(25, (uint64_t)isr25, KERNEL_CS, 0x8E);
    set_idt_entry(26, (uint64_t)isr26, KERNEL_CS, 0x8E);
    set_idt_entry(27, (uint64_t)isr27, KERNEL_CS, 0x8E);
    set_idt_entry(28, (uint64_t)isr28, KERNEL_CS, 0x8E);
    set_idt_entry(29, (uint64_t)isr29, KERNEL_CS, 0x8E);
    set_idt_entry(30, (uint64_t)isr30, KERNEL_CS, 0x8E);
    set_idt_entry(31, (uint64_t)isr31, KERNEL_CS, 0x8E);

    set_idt_entry(32, (uint64_t)isr32, KERNEL_CS, 0x8E);
    set_idt_entry(33, (uint64_t)isr33, KERNEL_CS, 0x8E);
    set_idt_entry(34, (uint64_t)isr34, KERNEL_CS, 0x8E);
    set_idt_entry(35, (uint64_t)isr35, KERNEL_CS, 0x8E);
    set_idt_entry(36, (uint64_t)isr36, KERNEL_CS, 0x8E);
    set_idt_entry(37, (uint64_t)isr37, KERNEL_CS, 0x8E);
    set_idt_entry(38, (uint64_t)isr38, KERNEL_CS, 0x8E);
    set_idt_entry(39, (uint64_t)isr39, KERNEL_CS, 0x8E);
    set_idt_entry(40, (uint64_t)isr40, KERNEL_CS, 0x8E);
    set_idt_entry(41, (uint64_t)isr41, KERNEL_CS, 0x8E);
    set_idt_entry(42, (uint64_t)isr42, KERNEL_CS, 0x8E);
    set_idt_entry(43, (uint64_t)isr43, KERNEL_CS, 0x8E);
    set_idt_entry(44, (uint64_t)isr44, KERNEL_CS, 0x8E);
    set_idt_entry(45, (uint64_t)isr45, KERNEL_CS, 0x8E);
    set_idt_entry(46, (uint64_t)isr46, KERNEL_CS, 0x8E);
    set_idt_entry(47, (uint64_t)isr47, KERNEL_CS, 0x8E);

    load_idt();
}
