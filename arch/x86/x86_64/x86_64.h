#ifndef X86_64_H
#define X86_64_H

#include <stdint.h>
#include <stdbool.h>

// Define instruction set flags
#define X86_64_INSTR_SET_SSE      (1U << 0)
#define X86_64_INSTR_SET_SSE2     (1U << 1)
#define X86_64_INSTR_SET_SSE3     (1U << 2)
#define X86_64_INSTR_SET_SSSE3    (1U << 3)
#define X86_64_INSTR_SET_SSE4_1   (1U << 4)
#define X86_64_INSTR_SET_SSE4_2   (1U << 5)
#define X86_64_INSTR_SET_AVX      (1U << 6)
#define X86_64_INSTR_SET_AVX2     (1U << 7)
#define X86_64_INSTR_SET_FMA      (1U << 8)
#define X86_64_INSTR_SET_FMA4     (1U << 9)
#define X86_64_INSTR_SET_FMA3     (1U << 10)
#define X86_64_INSTR_SET_AVX512F  (1U << 11)
#define X86_64_INSTR_SET_AVX512CD (1U << 12)
#define X86_64_INSTR_SET_AVX512ER (1U << 13)
#define X86_64_INSTR_SET_AVX512PF (1U << 14)
#define X86_64_INSTR_SET_AVX512EF (1U << 15)

// Define feature flags
#define X86_64_FEATURE_FP        (1U << 0)
#define X86_64_FEATURE_ASM       (1U << 1)
#define X86_64_FEATURE_AVX       (1U << 2)
#define X86_64_FEATURE_AES       (1U << 3)
#define X86_64_FEATURE_RDRND     (1U << 4)
#define X86_64_FEATURE_FMA       (1U << 5)
#define X86_64_FEATURE_CVT16     (1U << 6)
#define X86_64_FEATURE_MOVBE     (1U << 7)

// Structure for storing CPUID information
typedef struct {
    uint32_t vendor_id;
    uint32_t device_id;
    uint32_t revision;
    uint32_t features;
    uint32_t instruction_sets[4];
} x86_64_cpuid_t;

// Global variable for accessing CPUID information
x86_64_cpuid_t x86_64_cpuid;

#endif // X86_64_H
