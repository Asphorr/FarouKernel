#ifndef X86_H
#define X86_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    SSE = 1 << 0,
    SSE2 = 1 << 1,
    SSE3 = 1 << 2,
    SSSE3 = 1 << 3,
    SSE4_1 = 1 << 4,
    SSE4_2 = 1 << 5,
    AVX = 1 << 6,
    AVX2 = 1 << 7,
    FMA = 1 << 8,
    FMA4 = 1 << 9,
    FMA3 = 1 << 10,
    AVX512F = 1 << 11,
    AVX512CD = 1 << 12,
    AVX512ER = 1 << 13,
    AVX512PF = 1 << 14,
    AVX512EF = 1 << 15,
} x86_instruction_set;

typedef enum {
    FP = 1 << 0,
    ASM = 1 << 1,
    AVX = 1 << 2,
    AES = 1 << 3,
    RDRND = 1 << 4,
    FMA = 1 << 5,
    CVT16 = 1 << 6,
    MOVBE = 1 << 7,
} x86_feature;

struct x86_cpuid {
    uint32_t vendor_id;
    uint32_t device_id;
    uint32_t revision;
    uint32_t features;
    uint32_t instruction_sets[4];
};

extern const struct x86_cpuid x86_cpuid;

static inline bool has_feature(x86_feature feature) {
    return (x86_cpuid.features & feature) != 0;
}

static inline bool has_instruction_set(x86_instruction_set set) {
    return (x86_cpuid.instruction_sets[set / 32] & (1U << (set % 32))) != 0;
}

#endif /* X86_H */
