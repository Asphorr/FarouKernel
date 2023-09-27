#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

#define MAX_LENGTH 1024

void *malloc(size_t size) {
    void *ptr = NULL;
    if (size > 0) {
        ptr = (void *) malloc(size);
        if (ptr == NULL) {
            printf("Memory allocation failed\n");
            exit(EXIT_FAILURE);
        }
    }
    return ptr;
}

void free(void *ptr) {
    if (ptr != NULL) {
        free(ptr);
    }
}

void *realloc(void *ptr, size_t size) {
    void *new_ptr = NULL;
    if (size > 0) {
        new_ptr = (void *) realloc(ptr, size);
        if (new_ptr == NULL) {
            printf("Memory reallocation failed\n");
            exit(EXIT_FAILURE);
        }
    } else {
        free(ptr);
    }
    return new_ptr;
}

char *strdup(const char *src) {
    size_t len = strlen(src);
    char *dst = (char *) malloc(len + 1);
    memcpy(dst, src, len);
    dst[len] = '\0';
    return dst;
}

int strcmp(const char *s1, const char *s2) {
    while (*s1 && *s2) {
        if (*s1++ != *s2++) {
            break;
        }
    }
    return *s1 - *s2;
}

size_t strlen(const char *s) {
    size_t len = 0;
    while (*s++) {
        len++;
    }
    return len;
}

void *memset(void *s, int c, size_t n) {
    size_t i = 0;
    while (i < n) {
        ((char *) s)[i] = (char) c;
        i++;
    }
    return s;
}

void *memcpy(void *dest, const void *src, size_t n) {
    size_t i = 0;
    while (i < n) {
        ((char *) dest)[i] = ((char *) src)[i];
        i++;
    }
    return dest;
}

size_t strtok(char **s, const char *delim) {
    size_t len = strlen(*s);
    char *token = strchr(*s, delim[0]);
    if (token == NULL) {
        token = *s + len;
    }
    *token = '\0';
    return token - *s;
}

int atoi(const char *s) {
    int result = 0;
    while (*s >= '0' && *s <= '9') {
        result = result * 10 + *s - '0';
        s++;
    }
    return result;
}

double atof(const char *s) {
    double result = 0;
    while (*s >= '0' && *s <= '9') {
        result = result * 10 + *s - '0';
        s++;
    }
    return result;
}

int abs(int x) {
    if (x < 0) {
        return -x;
    }
    return x;
}

int round(double x) {
    return (int) floor(x + 0.5);
}

double sqrt(double x) {
    return floor(sqrt(x));
}

int pow(int x, int y) {
    int result = 1;
    while (y-- > 0) {
        result *= x;
    }
    return result;
}

int exp(int x) {
    int result = 1;
    while (x-- > 0) {
        result *= 2;
    }
    return result;
}

int log(int x) {
    int result = 0;
    while (x > 0) {
        result++;
        x /= 2;
    }
    return result;
}

int factorial(int n) {
    if (n == 0) {
        return 1;
    }
    return n * factorial(n-1);
}

int fibonacci(int n) {
    if (n == 0 || n == 1) {
        return n;
    }
    return fibonacci(n-1) + fibonacci(n-2);
}

int main() {
    // Testing the functions
    printf("The factorial of 5 is %d\n", factorial(5));
    printf("The fibonacci of 6 is %d\n", fibonacci(6));

    // Dynamic memory allocation
    int *ptr = (int *) malloc(sizeof(int));
    *ptr = 10;
    printf("The value of the dynamically allocated memory is %d\n", *ptr);

    // Reallocating memory
    ptr = (int *) realloc(ptr, sizeof(int) * 2);
    *ptr = 20;
    printf("The value of the reallocated memory is %d\n", *ptr);

    // Freeing memory
    free(ptr);

    return 0;
}
