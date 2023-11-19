#ifndef CPUID_H
#define CPUID_H

#include <stdint.h>

typedef struct {
  uint32_t eax;
  uint32_t ebx;
  uint32_t ecx;
  uint32_t edx;
} cpuid_t;

void cpuid(cpuid_t *info, uint32_t function) {
  __asm__ __volatile__ (
      "cpuid;"
      : "=a"(info->eax), "=b"(info->ebx), "=c"(info->ecx), "=d"(info->edx)
      : "a"(function), "c"(0)
  );
}

void cpuid_ex(cpuid_t *info, uint32_t function, uint32_t subfunction) {
  __asm__ __volatile__ (
      "cpuid;"
      : "=a"(info->eax), "=b"(info->ebx), "=c"(info->ecx), "=d"(info->edx)
      : "a"(function), "c"(subfunction)
  );
}

void cpuid_vendor(char vendor[13]) {
  cpuid_t info;
  cpuid(&info, 0);
  *(uint32_t*)(vendor) = info.ebx;
  *(uint32_t*)(vendor+4) = info.edx;
  *(uint32_t*)(vendor+8) = info.ecx;
  vendor[12] = '\0';
}

#endif // CPUID_H
