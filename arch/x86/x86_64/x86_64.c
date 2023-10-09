#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "x86_64.h"

// CPUID information
static const uint32_t vendor_id = 0x12345678; // Intel or AMD vendor ID
static const uint32_t device_id = 0x23456789; // Device ID for the CPU
static const uint32_t revision = 0x01234567; // Revision number for the CPU
static const uint32_t features = X86_64_FEATURE_FP | X86_64_FEATURE_ASM | X86_64_FEATURE_AVX | X86_64_FEATURE_AES | X86_64_FEATURE_RDRND | X86_64_FEATURE_FMA | X86_64_FEATURE_CVT16 | X86_64_FEATURE_MOVBE;
static const char* instruction_set_names[] = {"SSE", "SSE2", "SSE3", "SSSE3", "SSE4_1", "SSE4_2", "AVX", "AVX2", "FMA", "FMA4", "FMA3", "AVX512F", "AVX512CD", "AVX512ER", "AVX512PF", "AVX512EF"};

void print_cpuid_info(const struct x86_64_cpuid* cpuid) {
    printf("CPUID Information:\n");
    printf("  Vendor ID: %08x\n", cpuid->vendor_id);
    printf("  Device ID: %08x\n", cpuid->device_id);
    printf("  Revision: %08x\n", cpuid->revision);
    printf("  Features: %08x\n", cpuid->features);
    printf("  Instruction Sets: ");
    for (size_t i = 0; i < sizeof(cpuid->instruction_sets) / sizeof(cpuid->instruction_sets[0]); ++i) {
        if (cpuid->instruction_sets[i]) {
            printf("%s ", instruction_set_names[i]);
        }
    }
    putchar('\n');
}

int main() {
    struct x86_64_cpuid cpuid;

    // Get CPUID information from the processor
    get_cpuid(&cpuid);

    // Print out the CPUID information
    print_cpuid_info(&cpuid);

    return 0;
}
