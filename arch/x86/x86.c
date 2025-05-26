#ifndef X86_H
#define X86_H

#include <cstdint>
#include <array>
#include <unordered_map>

namespace x86 {

enum class InstructionSet : uint32_t {
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
    AVX512ER = 1 << 13,
    AVX512PF = 1 << 14,
    AVX512EF = 1 << 15
};

enum class Feature : uint32_t {
    FP = 1 << 0,
    ASM = 1 << 1,
    AVX = 1 << 2,
    AES = 1 << 3,
    RDRND = 1 << 4,
    FMA = 1 << 5,
    CVT16 = 1 << 6,
    MOVBE = 1 << 7
};

struct CPUID {
    uint32_t vendor_id;
    uint32_t device_id;
    uint32_t revision;
    uint32_t features;
    std::array<uint32_t, 4> instruction_sets;
};

class CPUInfo {
public:
    CPUInfo(const CPUID& cpuid) : cpuid_(cpuid) {}

    bool hasFeature(Feature feature) const {
        auto it = feature_cache_.find(feature);
        if (it == feature_cache_.end()) {
            bool result = (cpuid_.features & static_cast<uint32_t>(feature)) != 0;
            feature_cache_[feature] = result;
            return result;
        }
        return it->second;
    }

    bool hasInstructionSet(InstructionSet set) const {
        auto it = instruction_set_cache_.find(set);
        if (it == instruction_set_cache_.end()) {
            uint32_t index = static_cast<uint32_t>(set) / 32;
            uint32_t bit = static_cast<uint32_t>(set) % 32;
            bool result = (cpuid_.instruction_sets[index] & (1U << bit)) != 0;
            instruction_set_cache_[set] = result;
            return result;
        }
        return it->second;
    }

private:
    const CPUID& cpuid_;
    mutable std::unordered_map<Feature, bool> feature_cache_;
    mutable std::unordered_map<InstructionSet, bool> instruction_set_cache_;
};

} // namespace x86

#endif /* X86_H */
