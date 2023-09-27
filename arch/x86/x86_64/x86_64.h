#ifndef X86_64_H
#define X86_64_H

#include <stdint.h>
#include <stdbool.h>

/* Instruction sets */
#define X86_64_INSTR_SET_SSE   (1 << 0)
#define X86_64_INSTR_SET_SSE2  (1 << 1)
#define X86_64_INSTR_SET_SSE3  (1 << 2)
#define X86_64_INSTR_SET_SSSE3 (1 << 3)
#define X86_64_INSTR_SET_SSE4_1 (1 << 4)
#define X86_64_INSTR_SET_SSE4_2 (1 << 5)
#define X86_64_INSTR_SET_AVX   (1 << 6)
#define X86_64_INSTR_SET_AVX2  (1 << 7)
#define X86_64_INSTR_SET_FMA   (1 << 8)
#define X86_64_INSTR_SET_FMA4  (1 << 9)
#define X86_64_INSTR_SET_FMA3  (1 << 10)
#define X86_64_INSTR_SET_AVX512F (1 << 11)
#define X86_64_INSTR_SET_AVX512CD (1 << 12)
#define X86_64_INSTR_SET_AVX512ER (1 << 13)
#define X86_64_INSTR_SET_AVX512PF (1 << 14)
#define X86_64_INSTR_SET_AVX512EF (1 << 15)

/* Features */
#define X86_64_FEATURE_FP     (1 << 0)
#define X86_64_FEATURE_ASM    (1 << 1)
#define X86_64_FEATURE_AVX    (1 << 2)
#define X86_64_FEATURE_AES    (1 << 3)
#define X86_64_FEATURE_RDRND  (1 << 4)
#define X86_64_FEATURE_FMA    (1 << 5)
#define X86_64_FEATURE_CVT16  (1 << 6)
#define X86_64_FEATURE_MOVBE  (1 << 7)

struct x86_64_cpuid {
    uint32_t vendor_id;
    uint32_t device_id;
    uint32_t revision;
    uint32_t features;
    uint32_t instruction_sets[4];
};

extern struct x86_64_cpuid x86_64_cpuid;

#endif /* X86_64_H */
