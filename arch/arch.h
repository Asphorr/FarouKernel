// arch.hpp - Architectural definitions for the kernel

#pragma once

#include <cstdint>
#include <type_traits>

namespace kernel {

// CPU architecture specific definitions
constexpr auto kIsa = Isa::kX86_64;
constexpr auto kEndianness = Endianness::kLittle;
constexpr std::size_t kWordSize = sizeof(uint64_t); // 64 bits

// Address space layout
constexpr bool kAslr = true;

// Stack layout
constexpr int kStackGrowthDirection = 1; // Grows upwards

// Page table layout
constexpr std::size_t kPageTableEntries = 1024;
constexpr std::size_t kPageTableShift = 12;

// Virtual memory layout
constexpr std::size_t kVirtualMemoryStart = 0x10000000;
constexpr std::size_t kVirtualMemoryEnd = 0x20000000;

// Interrupt controller
constexpr bool kInterruptControllerEnabled = false;

// Timekeeping
constexpr std::size_t kTimekeepingFrequency = 1000;

// Console output
constexpr std::size_t kConsoleOutputBufferSize = 4096;

// Networking
constexpr bool kNetworkingEnabled = true;

// File system
constexpr FileSystemType kFileSystemType = FileSystemType::kFat32;

// Memory management
constexpr bool kMemoryManagementEnabled = true;

// Kernel debugging
constexpr bool kKernelDebuggingEnabled = false;

// Platform-specific definitions
using PlatformSpecificData = struct {
    uint32_t foo;
    uint64_t bar;
};

// Platform-specific instructions
constexpr char kPlatformSpecificInstructions[] = "\x0f\xc7\xc0";

// Platform-specific functions
inline void platform_specific_function() {}

} // namespace kernel
