#include <cstdio>
#include <iostream>
#include <memory>
#include <span>
#include <type_traits>
#include <utility>
#include <vector>

template <typename T>
struct my_allocator {
    static constexpr bool propagate_on_container_copy_assignment = false;
    static constexpr bool propagate_on_container_move_assignment = true;
    static constexpr bool propagate_on_container_swap = true;

    template <typename U>
    struct rebind {
        typedef my_allocator<U> other;
    };

    explicit my_allocator(const char *name) noexcept {}

    [[nodiscard]] auto allocate(size_t n) -> void * {
        return ::operator new[](n);
    }

    void deallocate(void *p, size_t n) noexcept {
        ::operator delete[](p, n);
    }
};

template <>
struct my_allocator<void> {
    static constexpr bool propagate_on_container_copy_assignment = false;
    static constexpr bool propagate_on_container_move_assignment = true;
    static constexpr bool propagate_on_container_swap = true;

    template <typename U>
    struct rebind {
        typedef my_allocator<U> other;
    };

    explicit my_allocator(const char *name) noexcept {}

    [[nodiscard]] auto allocate(size_t n) -> void * {
        return ::operator new[](n);
    }

    void deallocate(void *p, size_t n) noexcept {
        ::operator delete[](p, n);
    }
};

template <typename T>
using pmr_vector = std::vector<T, my_allocator<T>>;

int main() {
    pmr_vector<int> vec1{"my_allocator"};
    vec1.reserve(3);
    vec1.emplace_back(42);
    vec1.emplace_back(78);
    vec1.emplace_back(96);

    pmr_vector<int> vec2{std::move(vec1)};

    std::span<int> span1{vec1.data(), vec1.size()};
    std::span<int> span2{vec2.data(), vec2.size()};

    printf("Contents of vec1:\n");
    for (auto &element : span1) {
        printf("%d ", element);
    }
    printf("\n");

    printf("Contents of vec2:\n");
    for (auto &element : span2) {
        printf("%d ", element);
    }
    printf("\n");

    return 0;
}
