#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define MALLOC_OR_DIE(size) \
    ({ \
        void *ptr = malloc((size)); \
        if (!ptr) { \
            perror("malloc failed"); \
            abort(); \
        } \
        ptr; \
    })

#define FREE_OR_DIE(ptr) \
    do { \
        if ((ptr)) { \
            free((ptr)); \
            (ptr) = NULL; \
        } \
    } while (0)

#define PRINT_ERROR_AND_EXIT(message) \
    do { \
        fputs((message), stderr); \
        exit(1); \
    } while (0)

#define STRDUP_OR_DIE(str) \
    ({ \
        char *copy = strdup((str)); \
        if (!copy) { \
            perror("strdup failed"); \
            abort(); \
        } \
        copy; \
    })

#define REALLOC_OR_DIE(ptr, size) \
    ({ \
        void *new_ptr = realloc((ptr), (size)); \
        if (!new_ptr && !(ptr)) { \
            perror("realloc failed"); \
            abort(); \
        } \
        new_ptr; \
    })

#define MEMCMPY_OR_DIE(dest, src, n) \
    do { \
        if (memcmp((src), (dest), (n))) { \
            perror("memcpy failed"); \
            abort(); \
        } \
    } while (0)

#define MEMMOVE_OR_DIE(dest, src, n) \
    do { \
        if (memmove((src), (dest), (n))) { \
            perror("memmove failed"); \
            abort(); \
        } \
    } while (0)

#define MEMSET_OR_DIE(ptr, val, n) \
    do { \
        if (memset((ptr), (val), (n))) { \
            perror("memset failed"); \
            abort(); \
        } \
    } while (0)

#define BZERO_OR_DIE(ptr, n) \
    do { \
        if (bzero((ptr), (n))) { \
            perror("bzero failed"); \
            abort(); \
        } \
    } while (0)

#define QSORT_OR_DIE(base, num, size, compar) \
    do { \
        if (qsort((base), (num), (size), (compar))) { \
            perror("qsort failed"); \
            abort(); \
        } \
    } while (0)
