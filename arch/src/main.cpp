#ifndef ARCH_HPP
#define ARCH_HPP

// Include necessary headers
#include <stdint.h>
#include "device_header.h" // Replace with the actual header for your device

// Define architecture-specific constants
#define MEMORY_BASE_ADDRESS 0x10000000
#define MEMORY_SIZE 0x10000000
#define INTERRUPT_CONTROLLER_BASE_ADDRESS 0x20000000

// Define architecture-specific types
typedef uint32_t arch_type;
typedef uint32_t memory_address_t;
typedef uint32_t interrupt_number_t;
typedef uint32_t device_register_t;

// Declare architecture-specific functions
void arch_init();
void arch_interrupt_enable(interrupt_number_t interrupt_number);
void arch_interrupt_disable(interrupt_number_t interrupt_number);
void arch_memory_init();
void arch_memory_map(memory_address_t virtual_address, memory_address_t physical_address);
void arch_memory_unmap(memory_address_t virtual_address);

// Device operations
device_register_t arch_device_read(device_register_t device_register);
void arch_device_write(device_register_t device_register, device_register_t value);

#endif // ARCH_HPP
