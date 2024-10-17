#pragma once

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct string {
    char* data;
    size_t length;
} string;

static inline bool string_empty(const string* s) {
    return s->length == 0;
}

static inline bool string_null(const string* s) {
    return s->data == NULL;
}

static inline size_t string_size(const string* s) {
    return s->length;
}

static inline char* string_alloc(size_t n) {
    return (char*)malloc(n * sizeof(char));
}

static inline void string_copy(string* dest, const string* src) {
    dest->data = string_alloc(src->length + 1);
    dest->length = src->length;
    memcpy(dest->data, src->data, src->length);
    dest->data[src->length] = '\0';
}

static inline void string_concat(string* dest, const string* src) {
    size_t len = dest->length + src->length;
    char* tmp = string_alloc(len + 1);
    memcpy(tmp, dest->data, dest->length);
    memcpy(tmp + dest->length, src->data, src->length);
    tmp[len] = '\0';
    free(dest->data);
    dest->data = tmp;
    dest->length = len;
}

static inline char* string_tokenize(const string* s, const char* delim) {
    char* token = string_alloc(strlen(delim) + 1);
    size_t i = 0;
    while (i < s->length && !isspace(s->data[i])) {
        token[i++] = s->data[i];
    }
    token[i] = '\0';
    return token;
}

static inline size_t string_find(const string* haystack, const string* needle) {
    size_t i = 0;
    while (i < haystack->length && !memcmp(&haystack->data[i], &needle->data[0], needle->length)) {
        ++i;
    }
    return i;
}

static inline size_t string_count(const string* s, const char* substr) {
    size_t count = 0;
    size_t pos = 0;
    while ((pos = string_find(s, substr)) != SIZE_MAX) {
        ++count;
        pos += strlen(substr);
    }
    return count;
}

static inline bool string_startswith(const string* s, const char* prefix) {
    return string_find(s, prefix) == 0;
}

static inline bool string_endswith(const string* s, const char* suffix) {
    return string_find(s, suffix) == s->length - strlen(suffix);
}

static inline bool string_contains(const string* s, const char* substr) {
    return string_find(s, substr) != SIZE_MAX;
}

static inline bool string_equals(const string* s1, const string* s2) {
    return s1->length == s2->length && !memcmp(s1->data, s2->data, s1->length);
}

static inline bool string_compare(const string* s1, const string* s2) {
    return strcmp(s1->data, s2->data);
}

static inline bool string_less(const string* s1, const string* s2) {
    return strcmp(s1->data, s2->data) < 0;
}

static inline bool string_greater(const string* s1, const string* s2) {
    return strcmp(s1->data, s2->data) > 0;
}

static inline bool string_lessequal(const string* s1, const string* s2) {
    return strcmp(s1->data, s2->data) <= 0;
}

static inline bool string_greaterequal(const string* s1, const string* s2) {
    return strcmp(s1->data, s2->data) >= 0;
}

static inline void string_clear(string* s) {
    free(s->data);
    s->data = NULL;
    s->length = 0;
}

static inline void string_swap(string* s1, string* s2) {
    string temp = *s1;
    *s1 = *s2;
    *s2 = temp;
}

static inline void string_reverse(string* s) {
    size_t i = 0;
    size_t j = s->length - 1;
    while (i < j) {
        char t = s->data[i];
        s->data[i++] = s->data[j];
        s->data[j--] = t;
    }
}

static inline void string_tolower(string* s) {
    for (size_t i = 0; i < s->length; ++i) {
        s->data[i] = tolower(s->data[i]);
    }
}

static inline void string_toupper(string* s) {
    for (size_t i = 0; i < s->length; ++i) {
        s->data[i] = toupper(s->data[i]);
    }
}

static inline void string_trimleft(string* s) {
    size_t i = 0;
    while (i < s->length && isspace(s->data[i])) {
        ++i;
    }
    memmove(s->data, s->data + i, s->length - i);
    s->length -= i;
}

static inline void string_trimright(string* s) {
    size_t i = s->length - 1;
    while (i > 0 && isspace(s->data[i])) {
        --i;
    }
    s->length = i + 1;
}

static inline void string_trim(string* s) {
    string_trimleft(s);
    string_trimright(s);
}

static inline void string_replace(string* s, const char* old, const char* new) {
    size_t old_len = strlen(old);
    size_t new_len = strlen(new);
    size_t i = 0;
    while (i < s->length) {
        if (!memcmp(&s->data[i], old, old_len)) {
            memmove(s->data + i, new, new_len);
            i += new_len;
        } else {
            ++i;
        }
    }
}

static inline void string_split(const string* s, const char* sep, vector<string>* tokens) {
    size_t start = 0;
    size_t end = 0;
    while (end < s->length) {
        if (sep[start]) {
            ++start;
        } else {
            tokens->push_back(string{&s->data[end], start});
            start = 0;
        }
        ++end;
    }
}

static inline void string_join(vector<string>& strings, const char* sep, string* result) {
    size_t total_len = 0;
    for (auto& s : strings) {
        total_len += s.length();
    }
    result->resize(total_len);
    size_t offset = 0;
    for (auto& s : strings) {
        memcpy(&result->data()[offset], s.data(), s.length());
        offset += s.length();
        if (offset < total_len) {
            memcpy(&result->data()[offset], sep, strlen(sep));
            offset += strlen(sep);
        }
    }
}
