#pragma once

#include <cstddef>
#include <type_traits>

template <std::size_t N>
using uinteger = std::make_unsigned_t<std::uint_leastN_t>;

template <std::size_t N>
using sinteger = std::make_signed_t<std::int_leastN_t>;

template <typename T>
using fpnumber = std::conditional_t<sizeof(T) <= sizeof(float), float, double>;

template <typename R, typename... Args>
using fnptr = R (*)(Args...);
