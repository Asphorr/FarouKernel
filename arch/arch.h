#pragma once

#include <cstdint>
#include <type_traits>

namespace kernel {

enum class Isa : uint8_t { X86_64 };
enum class Endianness : uint8_t { Little };

struct ArchitectureDefinitions {
    static constexpr Isaa kIsa = Isa::X86_64;
    static constexpr Endianness kEndianness = Endianness::Little;
    static constexpr size_t kWordSize = sizeof(uint64_t);
};

struct AddressSpaceLayout {
    static constexpr bool kAslr = true;
};

struct StackLayout {
    static constexpr int kStackGrowthDirection = 1;
};

struct PageTableLayout {
    static constexpr size_t kPageTableEntries = 1024;
    static constexpr size_t kPageTableShift = 12;
};

struct VirtualMemoryLayout {
    static constexpr size_t kVirtualMemoryStart = 0x10000000;
    static constexpr size_t kVirtualMemoryEnd = 0x20000000;
};

struct InterruptController {
    static constexpr bool kInterruptControllerEnabled = false;
};

struct Timekeeping {
    static constexpr size_t kTimekeepingFrequency = 1000;
};

struct ConsoleOutput {
    static constexpr size_t kConsoleOutputBufferSize = 4096;
};

struct Networking {
    static constexpr bool kNetworkingEnabled = true;
};

struct FileSystem {
    enum class Type : uint8_t { FAT32 };
    static constexpr Type kFileSystemType = Type::FAT32;
};

struct MemoryManagement {
    static constexpr bool kMemoryManagementEnabled = true;
};

struct KernelDebugging {
    static constexpr bool kKernelDebuggingEnabled = false;
};

struct PlatformSpecificData {
    uint32_t foo;
    uint64_t bar;
};

struct PlatformSpecificInstructions {
    static constexpr char kPlatformSpecificInstructions[] = "\x0f\xc7\xc0";
};

struct PlatformSpecificFunctions {
    inline void platform_specific_function();
};

} // namespace kernel
