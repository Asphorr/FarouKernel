#include "string.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Function to set a block of memory to a specified value
void* memset(void* s, int c, size_t n) {
    unsigned char* p = s;
    while (n-- > 0) {
        *p++ = (unsigned char)c;
    }
    return s;
}

// Function to copy one string to another
char* strcpy(char* s1, const char* s2) {
    char* p = s1;
    while ((*p++ = *s2++) != '\0') {}
    return s1;
}

// Function to return the length of a string
size_t strlen(const char* s) {
    size_t len = 0;
    while (*s++ != '\0') {
        len++;
    }
    return len;
}

// Function to compare two strings
int strcmp(const char* s1, const char* s2) {
    while (*s1 == *s2 && *s1 != '\0') {
        s1++;
        s2++;
    }
    if (*s1 == '\0') {
        return 0;
    } else {
        return *s1 - *s2;
    }
}

// Function to compare a portion of two strings
int strncmp(const char* s1, const char* s2, size_t n) {
    while (n-- > 0) {
        if (*s1 == *s2) {
            s1++;
            s2++;
        } else {
            break;
        }
    }
    if (n == 0) {
        return 0;
    } else {
        return *s1 - *s2;
    }
}

// Function to find a character in a string
char* strchr(const char* s, int c) {
    while (*s != '\0') {
        if (*s == c) {
            return (char*)s;
        }
        s++;
    }
    return NULL;
}

// Function to find the last occurrence of a character in a string
char* strrchr(const char* s, int c) {
    char* p = NULL;
    while (*s != '\0') {
        if (*s == c) {
            p = (char*)s;
        }
        s++;
    }
    return p;
}

// Function to calculate the maximum number of bytes that can be read from a string without encountering a byte not in a given set
size_t strspn(const char* s, const char* accept) {
    size_t count = 0;
    while (*s != '\0') {
        if (strchr(accept, *s) != NULL) {
            count++;
        } else {
            break;
        }
        s++;
    }
    return count;
}

// Function to calculate the maximum number of bytes that can be read from a string without encountering a byte in a given set
size_t strcspn(const char* s, const char* reject) {
    size_t count = 0;
    while (*s != '\0') {
        if (strchr(reject, *s) == NULL) {
            count++;
        } else {
            break;
        }
        s++;
    }
    return count;
}

// Function to split a string into multiple tokens using a delimiter
char* strtok(char* s, const char* delim) {
    char* p = s;
    while (*p != '\0') {
        if (strchr(delim, *p) != NULL) {
            *p++ = '\0';
            return s;
        }
        p++;
    }
    return NULL;
}

// Function to append a string to another string
size_t strlcat(char* s1, const char* s2, size_t n) {
    size_t len = strlen(s1);
    strcpy(s1 + len, s2);
    return len + strlen(s2);
}

// Function to insert a substring into a string
size_t strlinsert(char* s1, const char* s2, size_t offset) {
    size_t len = strlen(s1);
    size_t slen = strlen(s2);
    memmove(s1 + offset + slen, s1 + offset, len - offset);
    memcpy(s1 + offset, s2, slen);
    return len + slen;
}

// Function to replace a substring in a string
size_t strlreplace(char* s1, const char* s2, const char* s3, size_t offset) {
    size_t len = strlen(s1);
    size_t slen = strlen(s2);
    size_t clen = strlen(s3);
    memmove(s1 + offset + clen, s1 + offset + slen, len - offset - slen);
    memcpy(s1 + offset, s3, clen);
    return len + clen - slen;
}

// Function to reverse a string
char* strrev(char* s) {
    size_t len = strlen(s);
    char* rev = malloc(len + 1);
    for (size_t i = 0; i < len; i++) {
        rev[i] = s[len - i - 1];
    }
    rev[len] = '\0';
    return rev;
}

// Function to uppercase a string
char* strupper(char* s) {
    size_t len = strlen(s);
    for (size_t i = 0; i < len; i++) {
        s[i] = toupper(s[i]);
    }
    return s;
}

// Function to lowercase a string
char* strlower(char* s) {
    size_t len = strlen(s);
    for (size_t i = 0; i < len; i++) {
        s[i] = tolower(s[i]);
    }
    return s;
}

// Function to trim whitespace from a string
char* strtrim(char* s) {
    size_t len = strlen(s);
    size_t start = 0;
    size_t end = len - 1;
    while (start <= end && isspace(s[start])) {
        start++;
    }
    while (end >= start && isspace(s[end])) {
        end--;
    }
    memmove(s, s + start, end - start + 1);
    s[end] = '\0';
    return s;
}

// Function to check whether a string is empty
int streq(const char* s) {
    return *s == '\0';
}

// Function to check whether two strings are equal
int strneq(const char* s1, const char* s2) {
    size_t len = strlen(s1);
    if (len != strlen(s2)) {
        return 0;
    }
    for (size_t i = 0; i < len; i++) {
        if (s1[i] != s2[i]) {
            return 0;
        }
    }
    return 1;
}

// Function to check whether a string contains another string
int strcontains(const char* s1, const char* s2) {
    size_t len = strlen(s1);
    size_t slen = strlen(s2);
    for (size_t i = 0; i < len; i++) {
        if (memcmp(s1 + i, s2, slen) == 0) {
            return 1;
        }
    }
    return 0;
}

// Function to check whether a string starts with another string
int strstartswith(const char* s1, const char* s2) {
    size_t len = strlen(s1);
    size_t slen = strlen(s2);
    if (slen > len) {
        return 0;
    }
    for (size_t i = 0; i < slen; i++) {
        if (s1[i] != s2[i]) {
            return 0;
        }
    }
    return 1;
}

// Function to check whether a string ends with another string
int streendswith(const char* s1, const char* s2) {
    size_t len = strlen(s1);
    size_t slen = strlen(s2);
    if (slen > len) {
        return 0;
    }
    for (size_t i = len - slen; i < len; i++) {
        if (s1[i] != s2[i - (len - slen)]) {
            return 0;
        }
    }
    return 1;
}

// Function to find the first occurrence of a substring in a string
size_t strindex(const char* s1, const char* s2) {
    size_t len = strlen(s1);
    size_t slen = strlen(s2);
    for (size_t i = 0; i < len; i++) {
        if (memcmp(s1 + i, s2, slen) == 0) {
            return i;
        }
    }
    return -1;
}

// Function to find the last occurrence of a substring in a string
size_t strrindex(const char* s1, const char* s2) {
    size_t len = strlen(s1);
    size_t slen = strlen(s2);
    for (size_t i = len - slen; i < len; i++) {
        if (memcmp(s1 + i, s2, slen) == 0) {
            return i;
        }
    }
    return -1;
}

// Function to split a string into multiple substrings
char** strsplit(const char* s1, const char* s2) {
    size_t len = strlen(s1);
    size_t slen = strlen(s2);
    char** splits = malloc((len + 1) / slen * sizeof(char*));
    char* p = s1;
    char* q = s1;
    int i = 0;
    while (q < s1 + len) {
        if (memcmp(q, s2, slen) == 0) {
            splits[i++] = p;
            p = q + slen;
        }
        q += slen;
    }
    splits[i] = p;
    return splits;
}

// Function to concatenate multiple substrings into a single string
char* strconcat(char** splits, size_t num_splits) {
    size_t total_len = 0;
    for (size_t i = 0; i < num_splits; i++) {
        total_len += strlen(splits[i]);
    }
    char* result = malloc(total_len + 1);
    char* p = result;
    for (size_t i = 0; i < num_splits; i++) {
        strcpy(p, splits[i]);
        p += strlen(splits[i]);
    }
    *p = '\0';
    return result;
}
