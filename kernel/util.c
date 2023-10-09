#include <iostream>
#include <memory>
#include <type_traits>
#include <utility>

namespace mylib {

template <typename T>
class UniquePtr : public std::unique_ptr<T> {
public:
    explicit UniquePtr() {}

    template <typename... Args>
    explicit UniquePtr(Args&&... args) : std::unique_ptr<T>(new T{args...}) {}
};

template <>
UniquePtr<void>::~UniquePtr() {
    if (*this) {
        delete[] get();
    }
}

template <typename T>
struct Deleter {
    static constexpr bool value = false;
};

template <typename T>
Deleter<T> deleter(T*) {
    return {};
}

template <typename T>
auto make_unique(size_t n) -> UniquePtr<T> {
    return UniquePtr<T>{new T[n]{}};
}

template <typename T>
void safe_delete(T* p) {
    if (p && !deleter(p)) {
        delete[] p;
    }
}

template <typename T>
void safe_free(T* p) {
    if (p && !deleter(p)) {
        free(p);
    }
}

template <typename T>
void print_error_and_exit(const char* message) {
    std::cerr << message << '\n';
    std::exit(EXIT_FAILURE);
}

} // namespace mylib
