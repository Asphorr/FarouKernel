#include <concepts>
#include <numbers>
#include <utility>

template <std::integral T>
using uinteger = std::make_unsigned_t<T>;

template <std::floating_point T>
using fpnumber = std::conditional_t<sizeof(T) <= sizeof(float), float, double>;

int main() {
    constexpr int x = 10;
    constexpr int y = -5;
    constexpr double z = 0.0;

    static_assert(std::is_same_v<decltype(x), uinteger<int>>);
    static_assert(std::is_same_v<decltype(y), uinteger<int>>);
    static_assert(std::is_same_v<decltype(z), fpnumber<double>>);

    std::cout << "x = " << x << ", y = " << y << ", z = " << z << '\n';

    return 0;
}
