#ifndef ARCH_X86_64_HPP
#define ARCH_X86_64_HPP

#include <cstdint>

// Define the size of a pointer
typedef uint64_t pointer_t;

// Define the size of a word
typedef uint64_t word_t;

// Define the size of a double word
typedef uint32_t dword_t;

// Define the size of a quad word
typedef uint16_t qword_t;

// Define the size of a half word
typedef uint8_t hword_t;

// Define the size of a byte
typedef uint8_t byte_t;

// Define the size of a nibble
typedef uint8_t nibble_t;

// Define the size of a bit
typedef bool bit_t;

// Define the x86_64 registers
enum class Register {
   RAX,
   RBX,
   RCX,
   RDX,
   RSI,
   RDI,
   RBP,
   RSP,
   R8,
   R9,
   R10,
   R11,
   R12,
   R13,
   R14,
   R15,
   RIP,
   EFLAGS,
   CS,
   SS,
   DS,
   ES,
   FS,
   GS,
   // Add more registers here
};

// Define the x86_64 instructions
enum class Instruction {
   MOV,
   ADD,
   SUB,
   MUL,
   DIV,
   AND,
   OR,
   XOR,
   NOT,
   NEG,
   INC,
   DEC,
   CMP,
   JMP,
   JE,
   JNE,
   JL,
   JLE,
   JG,
   JGE,
   CALL,
   RET,
   PUSH,
   POP,
   NOP,
   // Add more instructions here
};

// Define the x86_64 flags
enum class Flag {
   CF, // Carry Flag
   PF, // Parity Flag
   AF, // Auxiliary Carry Flag
   ZF, // Zero Flag
   SF, // Sign Flag
   TF, // Trap Flag
   IF, // Interrupt Enable Flag
   DF, // Direction Flag
   OF, // Overflow Flag
   // Add more flags here
};

#endif // ARCH_X86_64_HPP
