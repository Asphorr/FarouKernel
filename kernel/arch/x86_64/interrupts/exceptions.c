#include "exceptions.h"
#include "../drivers/screen.h"  // Assumes you have a function for screen output
#include <stdint.h>

#define NUM_EXCEPTIONS 32

// Enum for exception numbers
typedef enum {
    DIVIDE_ERROR = 0,
    DEBUG_EXCEPTION,
    NON_MASKABLE_INTERRUPT,
    BREAKPOINT,
    OVERFLOW,
    BOUND_RANGE_EXCEEDED,
    INVALID_OPCODE,
    DEVICE_NOT_AVAILABLE,
    DOUBLE_FAULT,
    COPROCESSOR_SEGMENT_OVERRUN,
    INVALID_TSS,
    SEGMENT_NOT_PRESENT,
    STACK_SEGMENT_FAULT,
    GENERAL_PROTECTION_FAULT,
    PAGE_FAULT,
    RESERVED_15,
    x87_FLOATING_POINT_EXCEPTION,
    ALIGNMENT_CHECK,
    MACHINE_CHECK,
    SIMD_FLOATING_POINT_EXCEPTION,
    VIRTUALIZATION_EXCEPTION,
    CONTROL_PROTECTION_EXCEPTION,
    RESERVED_22,
    RESERVED_23,
    RESERVED_24,
    RESERVED_25,
    RESERVED_26,
    RESERVED_27,
    HYPERVISOR_INJECTION_EXCEPTION,
    VMM_COMMUNICATION_EXCEPTION,
    SECURITY_EXCEPTION,
    RESERVED_31
} exception_t;

// Exception messages corresponding to the exception numbers
static const char *exception_messages[NUM_EXCEPTIONS] = {
    [DIVIDE_ERROR]                  = "Division By Zero",
    [DEBUG_EXCEPTION]               = "Debug Exception",
    [NON_MASKABLE_INTERRUPT]        = "Non-Maskable Interrupt",
    [BREAKPOINT]                    = "Breakpoint",
    [OVERFLOW]                      = "Overflow",
    [BOUND_RANGE_EXCEEDED]          = "Bound Range Exceeded",
    [INVALID_OPCODE]                = "Invalid Opcode",
    [DEVICE_NOT_AVAILABLE]          = "Device Not Available",
    [DOUBLE_FAULT]                  = "Double Fault",
    [COPROCESSOR_SEGMENT_OVERRUN]   = "Coprocessor Segment Overrun",
    [INVALID_TSS]                   = "Invalid TSS",
    [SEGMENT_NOT_PRESENT]           = "Segment Not Present",
    [STACK_SEGMENT_FAULT]           = "Stack-Segment Fault",
    [GENERAL_PROTECTION_FAULT]      = "General Protection Fault",
    [PAGE_FAULT]                    = "Page Fault",
    [RESERVED_15]                   = "Reserved",
    [x87_FLOATING_POINT_EXCEPTION]  = "x87 Floating-Point Exception",
    [ALIGNMENT_CHECK]               = "Alignment Check",
    [MACHINE_CHECK]                 = "Machine Check",
    [SIMD_FLOATING_POINT_EXCEPTION] = "SIMD Floating-Point Exception",
    [VIRTUALIZATION_EXCEPTION]      = "Virtualization Exception",
    [CONTROL_PROTECTION_EXCEPTION]  = "Control Protection Exception",
    [RESERVED_22]                   = "Reserved",
    [RESERVED_23]                   = "Reserved",
    [RESERVED_24]                   = "Reserved",
    [RESERVED_25]                   = "Reserved",
    [RESERVED_26]                   = "Reserved",
    [RESERVED_27]                   = "Reserved",
    [HYPERVISOR_INJECTION_EXCEPTION]= "Hypervisor Injection Exception",
    [VMM_COMMUNICATION_EXCEPTION]   = "VMM Communication Exception",
    [SECURITY_EXCEPTION]            = "Security Exception",
    [RESERVED_31]                   = "Reserved"
};

// Function to check if an exception pushes an error code
static inline int exception_has_error_code(uint8_t int_no) {
    switch (int_no) {
        case DOUBLE_FAULT:
        case INVALID_TSS:
        case SEGMENT_NOT_PRESENT:
        case STACK_SEGMENT_FAULT:
        case GENERAL_PROTECTION_FAULT:
        case PAGE_FAULT:
        case ALIGNMENT_CHECK:
        case CONTROL_PROTECTION_EXCEPTION:
        case SECURITY_EXCEPTION:
            return 1;
        default:
            return 0;
    }
}

// Exception handler function
void exception_handler(interrupt_frame *frame) {
    if (frame->int_no < NUM_EXCEPTIONS) {
        print("Exception: ");
        print(exception_messages[frame->int_no]);
        print("\n");

        // Print the error code if present
        if (exception_has_error_code(frame->int_no)) {
            print("Error Code: ");
            print_hex(frame->error_code);
            print("\n");
        }

        // Optionally, print register states here if available
        // print_registers(frame);

        print("System Halted!\n");

        // Infinite loop to halt the system
        for (;;);
    } else {
        // Handle unknown exceptions
        print("Unknown Exception!\n");
        for (;;);
    }
}

// Initialize exception handlers
void init_exception_handlers(void) {
    for (uint8_t i = 0; i < NUM_EXCEPTIONS; i++) {
        // Set the appropriate handler based on whether the exception has an error code
        if (exception_has_error_code(i)) {
            idt_set_gate(i, (uint64_t)exception_handler_with_error, 0x08, 0x8E);
        } else {
            idt_set_gate(i, (uint64_t)exception_handler_no_error, 0x08, 0x8E);
        }
    }
}

// Assembly stubs for exceptions without error codes
__attribute__((naked)) void exception_handler_no_error(void) {
    asm volatile(
        "cli\n"                  // Clear interrupts
        "pushq $0\n"             // Push dummy error code
        "pushq %rax\n"           // Push interrupt number (needs to be set appropriately)
        "jmp exception_handler_common\n"
    );
}

// Assembly stubs for exceptions with error codes
__attribute__((naked)) void exception_handler_with_error(void) {
    asm volatile(
        "cli\n"                  // Clear interrupts
        "pushq %rax\n"           // Push interrupt number (needs to be set appropriately)
        "jmp exception_handler_common\n"
    );
}

// Common handler called by the assembly stubs
void exception_handler_common(interrupt_frame *frame) {
    exception_handler(frame);
}
