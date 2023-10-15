#include <bit>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <exception>
#include <stdexcept>
#include <system_error>
#include <vector>

namespace cpuid {

struct FeatureInfo {
    constexpr FeatureInfo() {}
    explicit operator bool() const { return false; }
};

constexpr inline FeatureInfo& operator|=(FeatureInfo& lhs, const FeatureInfo& rhs) {
    lhs.flags |= rhs.flags;
    return lhs;
}

constexpr inline FeatureInfo operator|(const FeatureInfo& lhs, const FeatureInfo& rhs) {
    return FeatureInfo{lhs.flags | rhs.flags};
}

constexpr inline FeatureInfo operator~(const FeatureInfo& info) {
    return FeatureInfo{~info.flags};
}

constexpr inline FeatureInfo operator<<(const FeatureInfo& info, size_t shift) {
    return FeatureInfo{info.flags << shift};
}

constexpr inline FeatureInfo operator>>(const FeatureInfo& info, size_t shift) {
    return FeatureInfo{info.flags >> shift};
}

constexpr inline FeatureInfo& operator+=(FeatureInfo& lhs, const FeatureInfo& rhs) {
    lhs.flags += rhs.flags;
    return lhs;
}

constexpr inline FeatureInfo operator+(const FeatureInfo& lhs, const FeatureInfo& rhs) {
    return FeatureInfo{lhs.flags + rhs.flags};
}

constexpr inline FeatureInfo& operator*=(FeatureInfo& lhs, const FeatureInfo& rhs) {
    lhs.flags *= rhs.flags;
    return lhs;
}

constexpr inline FeatureInfo operator*(const FeatureInfo& lhs, const FeatureInfo& rhs) {
    return FeatureInfo{lhs.flags * rhs.flags};
}

constexpr inline FeatureInfo& operator/=(FeatureInfo& lhs, const FeatureInfo& rhs) {
    lhs.flags /= rhs.flags;
    return lhs;
}

constexpr inline FeatureInfo operator/(const FeatureInfo& lhs, const FeatureInfo& rhs) {
    return FeatureInfo{lhs.flags / rhs.flags};
}

constexpr inline FeatureInfo& operator%=(FeatureInfo& lhs, const FeatureInfo& rhs) {
    lhs.flags %= rhs.flags;
    return lhs;
}

constexpr inline FeatureInfo operator%(const FeatureInfo& lhs, const FeatureInfo& rhs) {
    return FeatureInfo{lhs.flags % rhs.flags};
}

constexpr inline FeatureInfo& operator^=(FeatureInfo& lhs, const FeatureInfo& rhs) {
    lhs.flags ^= rhs.flags;
    return lhs;
}

constexpr inline FeatureInfo operator^(const FeatureInfo& lhs, const FeatureInfo& rhs) {
    return FeatureInfo{lhs.flags ^ rhs.flags};
}

constexpr inline FeatureInfo& operator&=(FeatureInfo& lhs, const FeatureInfo& rhs) {
    lhs.flags &= rhs.flags;
    return lhs;
}

constexpr inline FeatureInfo operator&(const FeatureInfo& lhs, const FeatureInfo& rhs) {
    return FeatureInfo{lhs.flags & rhs.flags};
}

constexpr inline FeatureInfo& operator|=(FeatureInfo& lhs, const FeatureInfo& rhs) {
    lhs.flags |= rhs.flags;
    return lhs;
}

constexpr inline FeatureInfo operator|(const FeatureInfo& lhs, const FeatureInfo& rhs) {
    return FeatureInfo{lhs.flags | rhs.flags};
}

constexpr inline FeatureInfo& operator!=(FeatureInfo& lhs, const FeatureInfo& rhs) {
    lhs.flags != rhs.flags;
    return lhs;
}

constexpr inline FeatureInfo operator==(const FeatureInfo& lhs, const FeatureInfo& rhs) {
    return FeatureInfo{lhs.flags == rhs.flags};
}

constexpr inline FeatureInfo& operator>=(FeatureInfo& lhs, const FeatureInfo& rhs) {
    lhs.flags >= rhs.flags;
    return lhs;
}

constexpr inline FeatureInfo operator>=(const FeatureInfo& lhs, const FeatureInfo& rhs) {
    return FeatureInfo{lhs.flags >= rhs.flags};
}

constexpr inline FeatureInfo& operator<=(FeatureInfo& lhs, const FeatureInfo& rhs) {
    lhs.flags <= rhs.flags;
    return lhs;
}

constexpr inline FeatureInfo operator<=(const FeatureInfo& lhs, const FeatureInfo& rhs) {
    return FeatureInfo{lhs.flags <= rhs.flags};
}

constexpr inline FeatureInfo& operator>>(FeatureInfo& lhs, const FeatureInfo& rhs) {
    lhs.flags > rhs.flags;
    return lhs;
}

constexpr inline FeatureInfo operator>(const FeatureInfo& lhs, const FeatureInfo& rhs) {
    return FeatureInfo{lhs.flags > rhs.flags};
}

constexpr inline FeatureInfo& operator<(FeatureInfo& lhs, const FeatureInfo& rhs) {
    lhs.flags < rhs.flags;
    return lhs;
}

constexpr inline FeatureInfo operator<(const FeatureInfo& lhs, const FeatureInfo& rhs) {
    return FeatureInfo{lhs.flags < rhs.flags};
}

class FeatureDetector {
public:
    virtual ~FeatureDetector() = default;
    virtual void init() = 0;
    virtual FeatureInfo queryFeatures() = 0;
};

class X86CpuIdFeatureDetector final : public FeatureDetector {
private:
    struct EaxRegister {
        uint32_t eax;
        uint32_t ebx;
        uint32_t ecx;
        uint32_t edx;
    };

    struct EbxRegister {
        uint32_t eax;
        uint32_t ebx;
        uint32_t ecx;
        uint32_t edx;
    };

    struct EcxRegister {
        uint32_t eax;
        uint32_t ebx;
        uint32_t ecx;
        uint32_t edx;
    };

    struct EdxRegister {
        uint32_t eax;
        uint32_t ebx;
        uint32_t ecx;
        uint32_t edx;
    };

    union RegisterUnion {
        EaxRegister eax;
        EbxRegister ebx;
        EcxRegister ecx;
        EdxRegister edx;
    };

    template <typename Reg>
    static consteval auto readReg(Reg reg) {
        switch (reg) {
            case EaxRegister:
                return __builtin_cpuid_eax();
            case EbxRegister:
                return __builtin_cpuid_ebx();
            case EcxRegister:
                return __builtin_cpuid_ecx();
            case EdxRegister:
                return __builtin_cpuid_edx();
            default:
                throw std::runtime_error("Invalid register");
        }
    }

    template <typename Reg>
    static consteval auto writeReg(Reg reg, uint32_t value) {
        switch (reg) {
            case EaxRegister:
                __builtin_cpuid_eax(value);
                break;
            case EbxRegister:
                __builtin_cpuid_ebx(value);
                break;
            case EcxRegister:
                __builtin_cpuid_ecx(value);
                break;
            case EdxRegister:
                __builtin_cpuid_edx(value);
                break;
            default:
                throw std::runtime_error("Invalid register");
        }
    }

    template <typename Reg>
