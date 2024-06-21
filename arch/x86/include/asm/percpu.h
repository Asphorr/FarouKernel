#ifndef _ASM_X86_PERCPU_H
#define _ASM_X86_PERCPU_H

#include <asm/thread_info.h>

#define DECLARE_PER_CPU(type, name) \
    DECLARE_PER_CPU_SECTION(type, name, __per_cpu_start)

#define DECLARE_PER_CPU_SECTION(type, name, section) \
    extern type __per_cpu_data section

#define percpu_read(var) (*({ \
    typeof(var) __tmp; \
    asm volatile("mov %%gs:%P[1], %0" : "=r" (__tmp) : "0" (var)); \
    &__tmp; \
}))

#define percpu_read_begin() ({ \
    asm volatile("mov %%gs:%P[1], %%eax" : : : "memory"); \
})

#define percpu_read_end() ({ \
    asm volatile("mov %%eax, %%gs:%P[1]" : : : "memory"); \
})

#endif /* _ASM_X86_PERCPU_H */
