#include "arch_x86_64.hpp"
#include <cstdint>

// Define the AND instruction
void and_op(Register dest, Register src) {
    // Perform a bitwise AND operation on the source register and the destination register
    asm volatile (
        "and %%rax, %%rbx"
        : "=b" (dest)
        : "a" (src)
    );
}

// Define the OR instruction
void or_op(Register dest, Register src) {
    // Perform a bitwise OR operation on the source register and the destination register
    asm volatile (
        "or %%rax, %%rbx"
        : "=b" (dest)
        : "a" (src)
    );
}

// Define the XOR instruction
void xor_op(Register dest, Register src) {
    // Perform a bitwise XOR operation on the source register and the destination register
    asm volatile (
        "xor %%rax, %%rbx"
        : "=b" (dest)
        : "a" (src)
    );
}

// Define the NOT instruction
void not_op(Register dest) {
    // Perform a bitwise NOT operation on the destination register
    asm volatile (
        "not %%rax"
        : "=a" (dest)
    );
}

// Define the SHL instruction
void shl(Register dest, word_t value) {
    // Shift the value from the immediate to the left
    asm volatile (
        "shl %[value], %%rax"
        : "=a" (dest)
        : [value] "r" (value)
    );
}

// Define the SHR instruction
void shr(Register dest, word_t value) {
    // Shift the value from the immediate to the right
    asm volatile (
        "shr %[value], %%rax"
        : "=a" (dest)
        : [value] "r" (value)
    );
}

// Define the JMP instruction
void jmp(pointer_t dest) {
    // Jump to the destination memory location
    asm volatile (
        "jmp *%[dest]"
        :
        : [dest] "r" (dest)
    );
}

// Define the CMP instruction
void cmp(Register dest, Register src) {
    // Compare the value from the source register with the destination register
    asm volatile (
        "cmp %%rax, %%rbx"
        :
        : "a" (src), "b" (dest)
    );
}

// Define the CMP instruction
void cmp(Register dest, word_t value) {
    // Compare the value from the immediate with the destination register
    asm volatile (
        "cmp %[value], %%rax"
        :
        : [value] "r" (value), "a" (dest)
    );
}
