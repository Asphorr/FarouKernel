#include <bit>
#include <concepts>
#include <iostream>
#include <type_traits>

using namespace std;

// Define instruction set enum class
enum class InstructionSet : uint32_t {
    None = 0,
    SSE = 1 << 0,
    SSE2 = 1 << 1,
    SSE3 = 1 << 2,
    SSSE3 = 1 << 3,
    SSE4_1 = 1 << 4,
    SSE4_2 = 1 << 5,
    AVX = 1 << 6,
    AVX2 = 1 << 7,
    FMA = 1 << 8,
    FMA4 = 1 << 9,
    FMA3 = 1 << 10,
    AVX512F = 1 << 11,
    AVX512CD = 1 << 12,
};

// Define concept for unsigned integer types
template<typename T>
concept UnsignedInteger = requires(T t) {
    { t } -> std::convertible_to<std::size_t>;
};

// Define helper function for getting instruction set flags
constexpr auto getInstructionSetFlags() noexcept -> bitset<32> {
    return bitset<32>{static_cast<uint32_t>(InstructionSet::None)};
}

// Define helper function for checking instruction set flags
template<UnsignedInteger UIntType>
requires std::same_as<UIntType, uint32_t> || std::same_as<UIntType, uint64_t>
constexpr auto hasInstructionSetFlag(InstructionSet flag) noexcept -> bool {
    return (getInstructionSetFlags().to_ullong() & static_cast<uint32_t>(flag)) != 0;
}

int main() {
    // Example usage
    cout << "Has AVX support: " << hasInstructionSetFlag(InstructionSet::AVX) << endl;
    return 0;
}
