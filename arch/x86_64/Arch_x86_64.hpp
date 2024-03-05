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
   // Add more general-purpose registers
   R16,
   R17,
   R18,
   R19,
   R20,
   R21,
   R22,
   R23,
   R24,
   R25,
   R26,
   R27,
   R28,
   R29,
   R30,
   R31,
   // Add more segment registers
   HS,
   // Add more control registers
   CR0,
   CR1,
   CR2,
   CR3,
   CR4,
   CR8,
   // Add more debug registers
   DR0,
   DR1,
   DR2,
   DR3,
   DR4,
   DR5,
   DR6,
   DR7,
   // Add more floating-point registers
   ST0,
   ST1,
   ST2,
   ST3,
   ST4,
   ST5,
   ST6,
   ST7,
   // Add more MMX registers
   MM0,
   MM1,
   MM2,
   MM3,
   MM4,
   MM5,
   MM6,
   MM7,
   // Add more XMM registers
   XMM0,
   XMM1,
   XMM2,
   XMM3,
   XMM4,
   XMM5,
   XMM6,
   XMM7,
   XMM8,
   XMM9,
   XMM10,
   XMM11,
   XMM12,
   XMM13,
   XMM14,
   XMM15,
   // Add more YMM registers
   YMM0,
   YMM1,
   YMM2,
   YMM3,
   YMM4,
   YMM5,
   YMM6,
   YMM7,
   YMM8,
   YMM9,
   YMM10,
   YMM11,
   YMM12,
   YMM13,
   YMM14,
   YMM15
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
   // Add more arithmetic
