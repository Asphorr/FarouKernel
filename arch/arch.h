#ifndef ARCH_H
#define ARCH_H

// CPU architecture specific definitions

// Instruction set architecture (ISA)
#define ISA_X86_64 1

// Endianness
#define ENDIANNESS_LITTLE 1

// Word size
#define WORD_SIZE 64

// Address space layout
#define ASLR 1

// Stack layout
#define STACK_GROWTH_DIRECTION 1

// Page table layout
#define PAGE_TABLE_ENTRIES 1024
#define PAGE_TABLE_SHIFT 12

// Virtual memory layout
#define VIRTUAL_MEMORY_START 0x10000000
#define VIRTUAL_MEMORY_END 0x20000000

// Interrupt controller
#define INTERRUPT_CONTROLLER_APIC 1

// Timekeeping
#define TIMEKEEPING_FREQUENCY 1000

// Console output
#define CONSOLE_OUTPUT_BUFFER_SIZE 4096

// Networking
#define NETWORKING_ENABLED 1

// File system
#define FILE_SYSTEM_TYPE_FAT32 1

// Memory management
#define MEMORY_MANAGEMENT_ENABLED 1

// Kernel debugging
#define KERNEL_DEBUGGING_ENABLED 0

// Platform specific definitions

// Define the platform specific instructions
#define PLATFORM_SPECIFIC_INSTRUCTIONS \
    X86_INSTRUCTION(0x0f, 0xc7, 0xc0) \
    X86_INSTRUCTION(0x0f, 0xc7, 0xe0) \
    X86_INSTRUCTION(0x0f, 0xc7, 0xf0)

// Define the platform specific data structures
struct platform_specific_data {
    uint32_t foo;
    uint64_t bar;
};

// Define the platform specific functions
void platform_specific_function(void);

#endif // ARCH_H
