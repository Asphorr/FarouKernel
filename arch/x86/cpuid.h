#ifndef CPUID_H
#define CPUID_H

typedef struct {
  unsigned int eax;
  unsigned int ebx;
  unsigned int ecx;
  unsigned int edx;
} cpuid_t;

void cpuid(cpuid_t *info, unsigned int function) {
  __asm__ __volatile__ (
      "cpuid;"
      : "=a"(info->eax), "=b"(info->ebx), "=c"(info->ecx), "=d"(info->edx)
      : "a"(function)
  );
}

void cpuid_ex(cpuid_t *info, unsigned int function, unsigned int subfunction) {
  __asm__ __volatile__ (
      "cpuid;"
      : "=a"(info->eax), "=b"(info->ebx), "=c"(info->ecx), "=d"(info->edx)
      : "a"(function), "c"(subfunction)
  );
}

#endif // CPUID_H
