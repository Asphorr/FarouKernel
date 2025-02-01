#ifndef MULTIBOOT2_PARSER_H
#define MULTIBOOT2_PARSER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Multiboot2 constants
#define MULTIBOOT2_BOOTLOADER_MAGIC 0x36d76289
#define MULTIBOOT2_FLAG 0x00000000

// Ensure 8-byte alignment
#define ALIGN(x, a) (((x) + ((a) - 1)) & ~((a) - 1))

// Simple logging macros
#define LOG_INFO(fmt, ...) printf("[INFO] " fmt "\n", ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) printf("[ERROR] " fmt "\n", ##__VA_ARGS__)

// Multiboot2 header structure
struct __attribute__((packed)) multiboot2_header {
    uint32_t magic;
    uint32_t architecture;
    uint32_t header_length;
    uint32_t checksum;
};

// Multiboot2 tag structure
struct __attribute__((packed)) multiboot2_tag {
    uint32_t type;
    uint32_t size;
};

// Multiboot2 information header structure
struct __attribute__((packed)) multiboot2_info_header {
    uint32_t total_size;
    uint32_t reserved;
};

// Multiboot2 memory map entry structure
struct __attribute__((packed)) multiboot2_mmap_entry {
    uint64_t base_addr;
    uint64_t length;
    uint32_t type;
    uint32_t reserved;
};

// Multiboot2 tag types
enum multiboot2_tag_type {
    MULTIBOOT2_TAG_TYPE_END = 0,
    MULTIBOOT2_TAG_TYPE_CMDLINE = 1,
    MULTIBOOT2_TAG_TYPE_BOOT_LOADER_NAME = 2,
    MULTIBOOT2_TAG_TYPE_MMAP = 6
};

// Function prototypes
const struct multiboot2_tag* find_multiboot2_tag(uint32_t type, const void* multiboot_info);
void print_memory_map(const void* multiboot_info);
void print_bootloader_info(const void* multiboot_info);
void print_kernel_cmdline(const void* multiboot_info);
void parse_multiboot2_info(const void* multiboot_info);

#endif // MULTIBOOT2_PARSER_H
