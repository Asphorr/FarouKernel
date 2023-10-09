// #pragma once

#include <concepts>
#include <functional>
#include <numeric>
#include <utility>

template <auto N>
using uinteger = std::make_unsigned_t<std::uint_fastN_t>;

template <auto N>
using sinteger = std::make_signed_t<std::int_fastN_t>;

template <typename T>
using fpnumber = std::conditional_t<sizeof(T) <= sizeof(float), float, double>;

template <typename R, typename... Args>
using fnptr = std::add_pointer_t<R(Args...)>;
