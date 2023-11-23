#include "bootloader_utils.h"

void print_string(char* str) {
    // Print a string to the screen
    while (*str) {
        asm("mov $0x0e, %ah"); // BIOS teletype output
        asm("int $0x10"); // BIOS video interrupt
        str++;
    }
}

void print_hex(int num) {
    // Print a hexadecimal number to the screen
    asm("mov $0x0e, %ah"); // BIOS teletype output
    asm("int $0x10"); // BIOS video interrupt
}

void disk_load(char* buffer, int sector, int head, int cylinder, int drive) {
    // Load sectors from the disk
    asm("push %%es"); // Save ES
    asm("push %%bx"); // Save BX
    asm("mov $0x7c0, %%ax"); // Load the address of the buffer into ES:BX
    asm("mov %%ax, %%es");
    asm("xor %%bx, %%bx");
    asm("mov $2, %%ah"); // BIOS read sector function
    asm("mov $1, %%al"); // Read 1 sector
    asm("mov %0, %%cl"); // Load the sector number
    asm("mov %1, %%ch"); // Load the cylinder number
    asm("mov %2, %%dh"); // Load the head number
    asm("mov %3, %%dl"); // Load the drive number
    asm("int $0x13"); // BIOS disk interrupt
    asm("jc disk_error"); // Jump if carry flag is set
    asm("pop %%bx"); // Restore BX
    asm("pop %%es"); // Restore ES
    asm("ret");
}
