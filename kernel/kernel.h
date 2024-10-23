#pragma once

#include <concepts>
#include <type_traits>
#include <utility>

template<typename T>
struct MinMax {
    static constexpr auto min = std::numeric_limits<T>::min();
    static constexpr auto max = std::numeric_limits<T>::max();
};

using Byte = uint8_t;

inline namespace memory {
    template<typename T>
    concept Allocator = requires(T alloc) {
        typename T::value_type;
        typename T::pointer;
        typename T::reference;
        typename T::const_pointer;
        typename T::const_reference;
        typename T::size_type;
        typename T::difference_type;
        
        // allocation functions
        {alloc.allocate()} -> std::same_as<T::pointer>;
        {alloc.deallocate(nullptr)} -> std::same_as<void>;
        {alloc.construct(nullptr)} -> std::same_as<void>;
        {alloc.destroy(nullptr)} -> std::same_as<void>;
    };
    
    struct MallocAllocator {
        using value_type = void*;
        using pointer = void**;
        using reference = void&;
        using const_pointer = const void*;
        using const_reference = const void&;
        using size_type = size_t;
        using difference_type = ptrdiff_t;
        
        [[nodiscard]] pointer allocate() { return malloc(sizeof(value_type)); }
        void deallocate(pointer p) { free(p); }
        void construct(pointer p) {}
        void destroy(pointer p) {}
    };
}

namespace string {
    inline namespace utilities {
        template<typename CharT>
        class StringView {
            public:
                using traits_type = std::char_traits<CharT>;
                
                StringView() = default;
                explicit StringView(const CharT* s) : m_data{s}, m_length{traits_type::length(s)} {}
                StringView(const CharT* s, size_t length) : m_data{s}, m_length{length} {}
                
                bool empty() const { return m_length == 0; }
                size_t size() const { return m_length; }
                const CharT* data() const { return m_data; }
                
                friend bool operator==(StringView lhs, StringView rhs) {
                    return lhs.m_length == rhs.m_length && traits_type::compare(lhs.m_data, rhs.m_data, lhs.m_length) == 0;
                }
            
            private:
                const CharT* m_data = nullptr;
                size_t m_length = 0;
        };
    }
}

namespace chrono {
    using nanoseconds = std::chrono::nanoseconds;
    using microseconds = std::chrono::microseconds;
    using milliseconds = std::chrono::milliseconds;
    using seconds = std::chrono::seconds;
    using minutes = std::chrono::minutes;
    using hours = std::chrono::hours;
    
    template<typename Rep, typename Period>
    using duration = std::chrono::duration<Rep, Period>;
    
    template<typename Duration1, typename Duration2>
    using common_type = std::common_type_t<Duration1, Duration2>;
    
    template<typename Duration>
    using rep = typename Duration::rep;
    
    template<typename Duration>
    using period = typename Duration::period;
    
    template<typename Duration>
    using time_point = std::chrono::time_point<Duration>;
    
    template<typename Duration>
    using sys_time = std::chrono::sys_time<Duration>;
    
    template<typename Duration>
    using steady_clock = std::chrono::steady_clock<Duration>;
    
    template<typename Duration>
    using high_resolution_clock = std::chrono::high_resolution_clock<Duration>;
    
    template<typename Duration>
    using system_clock = std::chrono::system_clock<Duration>;
    
    template<typename Duration>
    using monotonic_clock = std::chrono::monotonic_clock<Duration>;
    
    template<typename Duration>
    using process_real_cpu_clock = std::chrono::process_real_cpu_clock<Duration>;
    
    template<typename Duration>
    using thread_cpu_clock = std::chrono::thread_cpu_clock<Duration>;
    
    template<typename Duration>
    using gpu_clock = std::chrono::gpu_clock<Duration>;
    
    template<typename Duration>
    using cpu_clock = std::chrono::cpu_clock<Duration>;
    
    template<typename Duration>
    using wall_clock = std::chrono::wall_clock<Duration>;
    
    template<typename Duration>
    using user_clock = std::chrono::user_clock<Duration>;
    
    template<typename Duration>
    using system_clock = std::chrono::system_clock<Duration>;
    
    template<typename Duration>
    using real_time_clock = std::chrono::real_time_clock<Duration>;
    
    template<typename Duration>
    using monotonic_clock = std::chrono::monotonic_clock<Duration>;
    
    template<typename Duration>
    using steady_clock = std::chrono::steady_clock<Duration>;
    
    template<typename Duration>
    using high_resolution_clock = std::chrono::high_resolution_clock<Duration>;
    
    template<typename Duration>
    using process_real_cpu_clock = std::chrono::process_real_cpu_clock<Duration>;
    
    template<typename Duration>
    using thread_cpu_clock = std::chrono::thread_cpu_clock<Duration>;
    
    template<typename Duration>
    using gpu_clock = std::chrono::gpu_clock<Duration>;
    
    template<typename Duration>
    using cpu_clock
