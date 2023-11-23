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
    { t.reallocate() };
};

// Implement a default mallocator
class DefaultMallocator {
public:
    void allocate() {
        std::cout << "DefaultMallocator::allocate()\n";
    }
    void deallocate() {
        std::cout << "DefaultMallocator::deallocate()\n";
    }
    void reallocate() {
        std::cout << "DefaultMallocator::reallocate()\n";
    }
};

// Implement a custom mallocator
class CustomMallocator {
public:
    explicit CustomMallocator(const char* name) : name_(name) {}

    void allocate() {
        std::cout << "CustomMallocator::allocate()\n";
    }
    void deallocate() {
        std::cout << "CustomMallocator::deallocate()\n";
    }
    void reallocate() {
        std::cout << "CustomMallocator::reallocate()\n";
    }
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
template <Mallocator T>
class MallocatorWrapper {
public:
    explicit MallocatorWrapper(T& mallocator) : mallocator_(mallocator) {}

    void allocate() {
        mallocator_.allocate();
    }

    void deallocate() {
        mallocator_.deallocate();
    }

    void reallocate() {
        mallocator_.reallocate();
    }
private:
    T& mallocator_;
};

// Define a helper function to create a unique pointer with a custom delete
template <typename T, Mallocator M>
auto make_unique_with_custom_delete(M& mallocator) {
    return std::unique_ptr<T, MallocatorWrapper<M>>(new T, MallocatorWrapper<M>(mallocator));
}

int main() {
    DefaultMallocator defaultAllocator;
    CustomMallocator customAllocator("my_mallocator");

    auto myUniquePtr = make_unique_with_custom_delete<int>(defaultAllocator);
    auto myOtherUniquePtr = make_unique_with_custom_delete<int>(customAllocator);

    // do something with the unique_ptrs...

    return 0;
}
