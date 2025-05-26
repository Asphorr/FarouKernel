#include <stdio.h>
#include "cpuid.h"

typedef struct {
    int sse;
    int sse2;
    int sse3;
    int ssse3;
    int sse4_1;
    int sse4_2;
    int avx;
    int avx2;
    int fma;
    int fma4;
    int fma3;
    int avx512f;
    int avx512cd;
    int avx512er;
    int avx512pf;
    int avx512ef;
} cpu_features_t;

// CPUID information
cpu_features_t cpuid;

// Initialize CPUID information
void init_cpuid(void) {
    // Get CPUID information
    cpuid.sse = cpuid_get_feature(CPUID_SSE);
    cpuid.sse2 = cpuid_get_feature(CPUID_SSE2);
    cpuid.sse3 = cpuid_get_feature(CPUID_SSE3);
    cpuid.ssse3 = cpuid_get_feature(CPUID_SSSE3);
    cpuid.sse4_1 = cpuid_get_feature(CPUID_SSE4_1);
    cpuid.sse4_2 = cpuid_get_feature(CPUID_SSE4_2);
    cpuid.avx = cpuid_get_feature(CPUID_AVX);
    cpuid.avx2 = cpuid_get_feature(CPUID_AVX2);
    cpuid.fma = cpuid_get_feature(CPUID_FMA);
    cpuid.fma4 = cpuid_get_feature(CPUID_FMA4);
    cpuid.fma3 = cpuid_get_feature(CPUID_FMA3);
    cpuid.avx512f = cpuid_get_feature(CPUID_AVX512F);
    cpuid.avx512cd = cpuid_get_feature(CPUID_AVX512CD);
    cpuid.avx512er = cpuid_get_feature(CPUID_AVX512ER);
    cpuid.avx512pf = cpuid_get_feature(CPUID_AVX512PF);
    cpuid.avx512ef = cpuid_get_feature(CPUID_AVX512EF);
}

// Print CPUID information
void print_cpuid(void) {
    printf("CPUID Information:\n");
    printf(" SSE: %d\n", cpuid.sse);
    printf(" SSE2: %d\n", cpuid.sse2);
    printf(" SSE3: %d\n", cpuid.sse3);
    printf(" SSSE3: %d\n", cpuid.ssse3);
    printf(" SSE4.1: %d\n", cpuid.sse4_1);
    printf(" SSE4.2: %d\n", cpuid.sse4_2);
    printf(" AVX: %d\n", cpuid.avx);
    printf(" AVX2: %d\n", cpuid.avx2);
    printf(" FMA: %d\n", cpuid.fma);
    printf(" FMA4: %d\n", cpuid.fma4);
    printf(" FMA3: %d\n", cpuid.fma3);
    printf(" AVX512F: %d\n", cpuid.avx512f);
    printf(" AVX512CD: %d\n", cpuid.avx512cd);
    printf(" AVX512ER: %d\n", cpuid.avx512er);
    printf(" AVX512PF: %d\n", cpuid.avx512pf);
    printf(" AVX512EF: %d\n", cpuid.avx512ef);
}

int main() {
    init_cpuid();
    print_cpuid();
    return 0;
}
