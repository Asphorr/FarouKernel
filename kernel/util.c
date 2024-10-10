#include <iostream>
#include <utility>
#include <vector>

// A helper class to check if a type is an array
template <typename T>
struct IsArrayHelper {
    static constexpr bool value = false;
};

template <typename T, size_t N>
struct IsArrayHelper<T[N]> {
    static constexpr bool value = true;
};

template <typename T>
inline constexpr bool IsArray = IsArrayHelper<T>::value;

// A unique pointer wrapper that supports both single objects and arrays
template <typename T>
class UniquePtr {
public:
    // Constructor from a raw pointer
    explicit UniquePtr(T* ptr) noexcept : _ptr(ptr), _isArray(IsArray<T>) {}

    // Move constructor
    UniquePtr(UniquePtr&& other) noexcept : _ptr(other._ptr), _isArray(other._isArray) {
        other._ptr = nullptr;
    }

    // Destructor
    ~UniquePtr() noexcept {
        if (_ptr) {
            if (_isArray) {
                delete[] _ptr;
            } else {
                delete _ptr;
            }
        }
    }

    // Copy assignment operator
    UniquePtr& operator=(UniquePtr other) noexcept {
        swap(_ptr, other._ptr);
        swap(_isArray, other._isArray);
        return *this;
    }

    // Dereference operators
    T& operator*() noexcept { return *_ptr; }
    const T& operator*() const noexcept { return *_ptr; }

private:
    T* _ptr;
    bool _isArray;
};

// Function to create a new instance of a given type
template <typename T>
[[nodiscard]] auto CreateInstance(size_t n) -> UniquePtr<T> {
    if (!IsArray<T>) {
        return UniquePtr<T>(new T());
    } else {
        return UniquePtr<T>(new T[n]);
    }
}

int main() {
    // Create two instances of different types
    auto u1 = CreateInstance<int>();
    auto u2 = CreateInstance<double>(7);

    // Print out the values in each array
    for (int i = 0; i < 5; ++i) {
        std::cout << "u1[" << i << "]: " << u1[i] << '\n';
    }
    for (int j = 0; j < 7; ++j) {
        std::cout << "u2[" << j << "]: " << u2[j] << '\n';
    }

    // Check if the pointers are arrays
    static_assert(IsArray<decltype(*u1)> == false);
    static_assert(IsArray<decltype(*u2)> == true);

    // Delete the instances when they go out of scope
    return 0;
}
