#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/mman.h>
#include <asm/io.h>
#include <asm/interrupt.h>

#define PAGE_SIZE 4096
#define BUFFER_SIZE 1024

volatile uint8_t *idt;
volatile uint8_t *gdt;

static inline void load_idt(uintptr_t address) {
    asm volatile("lidt (%0)" : : "r"(address));
}

static inline void load_gdt(uintptr_t address) {
    asm volatile("lgdt (%0)" : : "r"(address));
}

static inline void set_vector(uint8_t vector, uintptr_t address) {
    idt[vector] = (address & 0xFFFFFFFF) | 0x80000000;
}

static inline void remap_interrupts() {
    // Remap interrupt vectors to our custom handler
    set_vector(0x20, (uintptr_t)custom_handler);
    set_vector(0x21, (uintptr_t)custom_handler + 1);
    set_vector(0x22, (uintptr_t)custom_handler + 2);
    set_vector(0x23, (uintptr_t)custom_handler + 3);
}

static inline void enable_interrupts() {
    // Enable interrupts in the CPU
    asm volatile("sti");
}

static inline void disable_interrupts() {
    // Disable interrupts in the CPU
    asm volatile("cli");
}

static void custom_handler(struct interrupt_frame *frame) {
    // Get the interrupt number from the frame
    uint8_t vector = frame->vector;

    // Check if the interrupt is a software interrupt (vector == 0x20)
    if (vector == 0x20) {
        // If it is, check the error code to determine what to do
        uint32_t err_code = frame->error_code;

        // Handle different software interrupts here
        switch (err_code) {
            case 0x12345678:
                // Do something for this error code
                break;
            default:
                // Default handling for unknown errors
                break;
        }
    } else {
        // Handle hardware interrupts here
        switch (vector) {
            case 0x21:
                // Handle interrupt 0x21
                break;
            case 0x22:
                // Handle interrupt 0x22
                break;
            case 0x23:
                // Handle interrupt 0x23
                break;
        }
    }

    // Clear the interrupt flag in the CPU
    asm volatile("sti");
}

int main(void) {
    // Allocate memory for the IDT and GDT
    idt = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    gdt = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    // Set up the IDT and GDT
    for (int i = 0; i < 256; i++) {
        idt[i] = (i << 3) | 0x80000000;
    }

    // Load the IDT and GDT
    load_idt((uintptr_t)idt);
    load_gdt((uintptr_t)gdt);

    // Remap interrupt vectors to our custom handler
    remap_interrupts();

    // Enable interrupts in the CPU
    enable_interrupts();

    // Wait forever
    while (true) {
    // Handle incoming interrupts here
    asm volatile("sti");
}
