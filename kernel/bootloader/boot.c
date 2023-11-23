#include "boot.asm"

void main() {
    // Call the assembly functions
    asm("call start");
    asm("call load_kernel");
    asm("call switch_to_pm");
    asm("call start_protected_mode");
}

void load_kernel() {
    // Load the kernel from disk
    asm("mov $0x1000, %bx"); // Load the kernel at address 0x1000
    asm("mov $2, %dh"); // Load 2 sectors (we assume the kernel is that big)
    asm("mov [BOOT_DRIVE], %dl"); // Load from the boot drive
    asm("call disk_load");
}

void switch_to_pm() {
    // Switch to protected mode
    asm("cli"); // Disable interrupts
    asm("lgdt [gdt_descriptor]"); // Load the GDT descriptor
    asm("mov $0x3, %eax"); // Set the first bit of CR0 to switch to protected mode
    asm("or %eax, %cr0");
    asm("jmp $0x08, $start_protected_mode"); // Far jump to our 32-bit code
}

void start_protected_mode() {
    // Set up the data segments for protected mode
    asm("mov $0x10, %ax");
    asm("mov %ax, %ds");
    asm("mov %ax, %ss");
    asm("mov %ax, %es");
    asm("mov %ax, %fs");
    asm("mov %ax, %gs");

    // Jump to the kernel
    asm("jmp $0x1000");
}
