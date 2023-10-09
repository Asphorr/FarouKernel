#include <concepts>
#include <iostream>
#include <memory>
#include <string>
#include <tuple>
#include <type_traits>
#include <variant>

// Define a concept for a mallocator
template <typename T>
concept Mallocator = requires (T t) {
    { t.allocate() };
    { t.deallocate() };
};

// Implement a default mallocator
class DefaultMallocator {
public:
    void allocate();
    void deallocate();
};

// Implement a custom mallocator
class CustomMallocator {
public:
    explicit CustomMallocator(const char* name) : name_(name) {}

    void allocate();
    void deallocate();
private:
    const char* name_;
};

// Define a trait to determine whether a given type is a mallocator
template <typename T>
struct IsMallocator : public std::false_type {};

template <>
struct IsMallocator<DefaultMallocator> : public std::true_type {};

template <>
struct IsMallocator<CustomMallocator> : public std::true_type {};

// Define a helper class to wrap around a mallocator
template <typename T>
requires IsMallocator<T>::value
class MallocatorWrapper {
public:
    explicit MallocatorWrapper(T& mallocator) : mallocator_(mallocator) {}

    void allocate() {
        mallocator_.allocate();
    }

    void deallocate() {
        mallocator_.deallocate();
    }
private:
    T& mallocator_;
};

// Define a helper function to create a unique pointer with a custom delete
template <typename T>
auto make_unique_with_custom_delete(const char* name) -> std::unique_ptr<T, MallocatorWrapper<decltype(detail::getMallocator<T>(name))>> {
    return std::unique_ptr<T, MallocatorWrapper<decltype(detail::getMallocator<T>(name))>>{detail::getMallocator<T>(name).allocate(), MallocatorWrapper<decltype(detail::getMallocator<T>(name))>{}};
}

int main() {
    auto defaultAllocator = detail::getMallocator<int>();
    auto customAllocator = detail::getMallocator<int>("my_mallocator");

    auto myUniquePtr = make_unique_with_custom_delete<int>(defaultAllocator);
    auto myOtherUniquePtr = make_unique_with_custom_delete<int>(customAllocator);

    // do something with the unique_ptrs...

    return 0;
}
