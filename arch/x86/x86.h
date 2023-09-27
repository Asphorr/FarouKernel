#ifndef X86_H
#define X86_H

// CPUID information
extern unsigned int cpuid[4];

// CPU feature flags
#define CPUID_SSE		(1 << 0)
#define CPUID_SSE2	(1 << 1)
#define CPUID_SSE3	(1 << 2)
#define CPUID_SSSE3	(1 << 3)
#define CPUID_SSE4_1	(1 << 4)
#define CPUID_SSE4_2	(1 << 5)
#define CPUID_AVX		(1 << 6)
#define CPUID_AVX2	(1 << 7)
#define CPUID_FMA		(1 << 8)
#define CPUID_FMA4	(1 << 9)
#define CPUID_FMA3	(1 << 10)
#define CPUID_AVX512F	(1 << 11)
#define CPUID_AVX512CD	(1 << 12)
#define CPUID_AVX512ER	(1 << 13)
#define CPUID_AVX512PF	(1 << 14)
#define CPUID_AVX512EF	(1 << 15)

// Instruction Set Architecture (ISA)
typedef struct {
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
} isa_registers_t;

// Read CR0
static inline uint32_t read_cr0(void) {
    uint32_t cr0;
    __asm__("mov %cr0, %0" : "=r"(cr0));
    return cr0;
}

// Write CR0
static inline void write_cr0(uint32_t value) {
    __asm__("mov %0, %cr0" ::"r"(value));
}

// Read CR4
static inline uint32_t read_cr4(void) {
    uint32_t cr4;
    __asm__("mov %cr4, %0" : "=r"(cr4));
    return cr4;
}

// Write CR4
static inline void write_cr4(uint32_t value) {
    __asm__("mov %0, %cr4" ::"r"(value));
}

// Endianness
typedef enum {
    LITTLE_ENDIAN = 0,
    BIG_ENDIAN = 1
} endianness_t;

// Word size
typedef enum {
    WORD_SIZE_32 = 0,
    WORD_SIZE_64 = 1
} word_size_t;

// Address space layout
typedef enum {
    ADDRESS_SPACE_LAYOUT_FLAT = 0,
    ADDRESS_SPACE_LAYOUT_SEGMENTED = 1
} address_space_layout_t;

// Stack layout
typedef enum {
    STACK_GROWTH_DIRECTION_DOWN = 0,
    STACK_GROWTH_DIRECTION_UP = 1
} stack_growth_direction_t;

// Page table layout
typedef enum {
    PAGE_TABLE_ENTRIES_4KB = 0,
    PAGE_TABLE_ENTRIES_2MB = 1,
    PAGE_TABLE_ENTRIES_4MB = 2,
    PAGE_TABLE_ENTRIES_8MB = 3
} page_table_entries_t;

// Virtual memory layout
typedef enum {
    VIRTUAL_MEMORY_START_4GB = 0,
    VIRTUAL_MEMORY_START_2GB = 1,
    VIRTUAL_MEMORY_START_1GB = 2
} virtual_memory_start_t;

// Interrupt controller
typedef enum {
    INTERRUPT_CONTROLLER_PIC = 0,
    INTERRUPT_CONTROLLER_APIC = 1
} interrupt_controller_t;

// Memory model
typedef enum {
    MEMORY_MODEL_Flat = 0,
    MEMORY_MODEL_Segmented = 1
} memory_model_t;

// CPU features
typedef struct {
    uint32_t sse;
    uint32_t sse2;
    uint32_t sse3;
    uint32_t ssse3;
    uint32_t sse4_1;
    uint32_t sse4_2;
    uint32_t avx;
    uint32_t avx2;
    uint32_t fma;
    uint32_t fma4;
    uint32_t fma3;
    uint32_t avx512f;
    uint32_t avx512cd;
    uint32_t avx512er;
    uint32_t avx512pf;
    uint32_t avx512ef;
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
    printf("  SSE: %d\n", cpuid.sse);
    printf("  SSE2: %d\n", cpuid.sse2);
    printf("  SSE3: %d\n", cpuid.sse3);
    printf("  SSSE3: %d\n", cpuid.ssse3);
    printf("  SSE4.1: %d\n", cpuid.sse4_1);
    printf("  SSE4.2: %d\n", cpuid.sse4_2);
    printf("  AVX: %d\n", cpuid.avx);
    printf("  AVX2: %d\n", cpuid.avx2);
    printf("  FMA: %d\n", cpuid.fma);
    printf("  FMA4: %d\n", cpuid.fma4);
      FMA3: %d\n", cpuid.fma3);
  printf("  AVX512F: %d\n", cpuid.avx512f);
  printf("  AVX512CD: %d\n", cpuid.avx512cd);
  printf("  AVX512ER: %d\n", cpuid.avx512er);
  printf("  AVX512PF: %d\n", cpuid.avx512pf);
  printf("  AVX512EF: %d\n", cpuid.avx512ef);
}

int main() {
  init_cpuid();
  print_cpuid();
  return 0;
}
