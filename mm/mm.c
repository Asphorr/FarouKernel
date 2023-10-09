#include <iostream>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

// A simple mallocator implementation
class Mallocator {
public:
    // Create a new instance of the mallocator
    static std::shared_ptr<Mallocator> create() {
        return std::make_shared<Mallocator>();
    }

    // Allocate memory using the mallocator
    void* allocate(size_t size) {
        return ::operator new(size);
    }

    // Free allocated memory
    void free(void* ptr) {
        ::operator delete(ptr);
    }
};

// A custom mallocator implementation that uses mmap and munmap
class CustomMallocator : public Mallocator {
public:
    // Constructor takes a string argument representing the name of the allocator
    explicit CustomMallocator(const char* name) : _name{name} {}

    // Override the allocate method to use mmap instead of the default operator new
    void* allocate(size_t size) override {
        auto result = ::mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (result == MAP_FAILED) {
            throw std::bad_alloc();
        }
        return result;
    }

    // Override the free method to use munmap instead of the default operator delete
    void free(void* ptr) override {
        ::munmap(ptr, 0);
    }
private:
    const char* _name;
};

// A utility function to create a unique_ptr with a custom deleter
template <typename T>
auto make_unique_with_custom_delete(Mallocator& allocator) {
    struct Deleter {
        void operator()(T* p) {
            p->~T();
            allocator.free(p);
        }
    };
    return std::unique_ptr<T, Deleter>{static_cast<T*>(allocator.allocate(sizeof(T)))};
}

int main() {
    // Create a shared pointer to the default mallocator
    auto defaultAllocator = Mallocator::create();

    // Create a shared pointer to the custom mallocator
    auto customAllocator = CustomMallocator::create("my_mallocator");

    // Create a unique_ptr with a custom deleter using the default mallocator
    auto myUniquePtr = make_unique_with_custom_delete<MyClass>(defaultAllocator);

    // Create another unique_ptr with a custom deleter using the custom mallocator
    auto myOtherUniquePtr = make_unique_with_custom_delete<MyClass>(customAllocator);

    // Do something with the unique_ptrs...

    // When they go out of scope, the custom deleters will automatically call the correct free functions
    return 0;
}
