#include <iostream>
#include <utility>
#include <cstddef>
#include <type_traits>

// =======================
// UniquePtr для одиночного T
// =======================
template<typename T>
class UniquePtr {
    static_assert(!std::is_array_v<T>, "Use UniquePtr<T[]> for arrays");
public:
    explicit UniquePtr(T* ptr = nullptr) noexcept : ptr_(ptr) {}
    ~UniquePtr() noexcept { delete ptr_; }

    UniquePtr(const UniquePtr&) = delete;
    UniquePtr& operator=(const UniquePtr&) = delete;

    UniquePtr(UniquePtr&& o) noexcept : ptr_(o.ptr_) { o.ptr_ = nullptr; }
    UniquePtr& operator=(UniquePtr&& o) noexcept {
        if (this != &o) {
            reset(o.release());
        }
        return *this;
    }

    T* get() const noexcept { return ptr_; }
    T* release() noexcept { T* tmp = ptr_; ptr_ = nullptr; return tmp; }
    void reset(T* ptr = nullptr) noexcept {
        if (ptr_ != ptr) {
            delete ptr_;
            ptr_ = ptr;
        }
    }
    void swap(UniquePtr& o) noexcept { std::swap(ptr_, o.ptr_); }

    T& operator*()  const noexcept { return *ptr_; }
    T* operator->() const noexcept { return ptr_; }
    explicit operator bool() const noexcept { return ptr_ != nullptr; }

private:
    T* ptr_;
};

// =======================
// Специализация UniquePtr для T[]
// =======================
template<typename T>
class UniquePtr<T[]> {
public:
    explicit UniquePtr(T* ptr = nullptr) noexcept : ptr_(ptr) {}
    ~UniquePtr() noexcept { delete[] ptr_; }

    UniquePtr(const UniquePtr&) = delete;
    UniquePtr& operator=(const UniquePtr&) = delete;

    UniquePtr(UniquePtr&& o) noexcept : ptr_(o.ptr_) { o.ptr_ = nullptr; }
    UniquePtr& operator=(UniquePtr&& o) noexcept {
        if (this != &o) {
            reset(o.release());
        }
        return *this;
    }

    T* get() const noexcept { return ptr_; }
    T* release() noexcept { T* tmp = ptr_; ptr_ = nullptr; return tmp; }
    void reset(T* ptr = nullptr) noexcept {
        if (ptr_ != ptr) {
            delete[] ptr_;
            ptr_ = ptr;
        }
    }
    void swap(UniquePtr& o) noexcept { std::swap(ptr_, o.ptr_); }

    T& operator[](std::size_t i) const noexcept { return ptr_[i]; }
    explicit operator bool() const noexcept { return ptr_ != nullptr; }

private:
    T* ptr_;
};

// =======================
// MakeUnique-обёртка
// =======================
template<typename T, typename... Args>
auto MakeUnique(Args&&... args) {
    if constexpr (std::is_array_v<T>) {
        static_assert(std::extent_v<T> == 0, "Use MakeUnique<T[]>(n) for arrays");
        using U = std::remove_extent_t<T>;
        return UniquePtr<U[]>(new U[std::forward<Args>(args)...]);
    } else {
        return UniquePtr<T>(new T(std::forward<Args>(args)...));
    }
}

// =======================
// Демонстрация
// =======================
int main() {
    // 1) Одиночный объект
    auto u1 = MakeUnique<int>( );
    *u1 = 42;
    std::cout << "*u1 = " << *u1 << "\n\n";

    // 2) Массив из 7 элементов
    auto u2 = MakeUnique<int[]>(7);
    for (size_t i = 0; i < 7; ++i)
        u2[i] = static_cast<int>(i * i);
    for (size_t i = 0; i < 7; ++i)
        std::cout << "u2[" << i << "] = " << u2[i] << "\n";

    return 0;
}
