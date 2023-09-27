#ifndef _STRING_H
#define _STRING_H

#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>

// Function prototypes
void* memset(void* s, int c, size_t n);
char* strcpy(char* s1, const char* s2);
size_t strlen(const char* s);
int strcmp(const char* s1, const char* s2);
int strncmp(const char* s1, const char* s2, size_t n);
char* strchr(const char* s, int c);
char* strrchr(const char* s, int c);
size_t strspn(const char* s, const char* accept);
size_t strcspn(const char* s, const char* reject);
char* strtok(char* s, const char* delim);
size_t strlcat(char* s1, const char* s2, size_t n);
size_t strlcpy(char* s1, const char* s2, size_t n);

// Macros
#define STRING_EMPTY   ""
#define STRING_NULL    NULL
#define STRING_SIZE     sizeof(char*)
#define STRING_ALLOC   malloc(sizeof(char) * (n + 1))
#define STRING_COPY    strcpy(s1, s2)
#define STRING_CAT     strcat(s1, s2)
#define STRING_TOKEN   strtok(s1, delim)

#endif // _STRING_H
