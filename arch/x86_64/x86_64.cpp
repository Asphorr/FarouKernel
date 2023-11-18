#include <iostream>
#include <string>
#include <vector>
#include <bitset>
#include <cmath>
#include <algorithm>
#include <numeric>

// Define a type alias for a bitset representing a set of instruction sets
using InstructionSetBitset = std::bitset<16>;

// Define a struct to hold CPUID information
struct CpuIdInfo {
    uint32_t vendorId;
    uint32_t deviceId;
    uint32_t revision;
    uint32_t features;
    InstructionSetBitset instructionSets;
};

// Define a function to get CPUID information from the processor
[[gnu::always_inline]] inline void get_cpuid(CpuIdInfo& info) {
    uint32_t eax, ebx, ecx, edx;

    // Call CPUID with EAX=0 to get the highest supported function parameter
    asm volatile("cpuid"
                 : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                 : "a"(0));

    // Store the vendor ID
    info.vendorId = ebx;
    info.deviceId = edx;
    info.revision = ecx;

    // Call CPUID with EAX=1 to get the feature information
    asm volatile("cpuid"
                 : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                 : "a"(1));

    // Store the feature information
    info.features = edx;

    // Check for instruction set support
    if (ecx & (1 << 25)) {
        info.instructionSets.set(0);  // SSE
    }
    if (ecx & (1 << 26)) {
        info.instructionSets.set(1);  // SSE2
    }
    if (ecx & (1 << 28)) {
        info.instructionSets.set(2);  // SSE3
    }
    // Add more checks for other instruction sets as needed
}

// Define a function to print out CPUID information
void printCpuIdInfo(const CpuIdInfo& info) {
    // Use std::format to create a formatted string
    auto cpuidString = std::format("CPUID Information:\n\nVendor ID: {:x}\nDevice ID: {:x}\nRevision: {:x}\nFeatures: {:x}",
                                   info.vendorId, info.deviceId, info.revision, info.features);
    
    // Iterate over the instruction sets and append them to the string
    for (auto&& [index, instructionSet] : enumerate(info.instructionSets)) {
        if (instructionSet) {
            cpuidString += std::format("\nInstruction Set {} ({})", index + 1, instructionSet);
        }
    }
    
    // Output the final string
    std::cout << cpuidString << std::endl;
}

int main() {
    // Create a CpuIdInfo instance
    CpuIdInfo info{};
    
    // Get CPUID information from the processor
    get_cpuid(info);
    
    // Print out the CPUID information
    printCpuIdInfo(info);
    
    return 0;
}
