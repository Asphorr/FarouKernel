#include <iostream>
#include <cstddef>
#include <memory>
#include <utility>

template <typename T>
using unique_ptr = std::unique_ptr<T>;

template <typename T>
inline unique_ptr<T> make_unique(size_t n) {
    return unique_ptr<T>{new T[n]{}};
}

template <typename T>
inline void safe_delete(T* p) {
    if (p != nullptr) {
        delete[] p;
    }
}

template <typename T>
inline void safe_free(T* p) {
    if (p != nullptr) {
        free(p);
    }
}

template <typename T>
inline void print_error_and_exit(const char* message) {
    std::cerr << message << '\n';
    std::exit(EXIT_FAILURE);
}
