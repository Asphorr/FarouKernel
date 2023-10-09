#pragma once

#include <cstdint>

namespace my {
    // Alias for unsigned integers
    template<size_t N>
    using uinteger = std::uint_fastN_t;
    
    // Alias for signed integers
    template<size_t N>
    using sinteger = std::int_fastN_t;
    
    // Alias for floating point numbers
    template<typename T>
    using fpnumber = typename std::conditional<sizeof(T) == sizeof(float), float, double>::type;
    
    // Alias for function pointers
    template<typename R, typename... Args>
    using fnptr = R(*)(Args...);
}
