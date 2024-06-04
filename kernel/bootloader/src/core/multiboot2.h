#ifndef MULTIBOOT2_H
#define MULTIBOOT2_H

#include <stdint.h>

// Multiboot 2 header structure
struct multiboot2_header {
    uint32_t magic;
    uint32_t architecture;
    uint32_t header_length;
    uint32_t checksum;
} __attribute__((packed));

// Multiboot 2 information structure
struct multiboot2_info_header {
    uint32_t total_size;
    uint32_t reserved;
};

// Multiboot 2 tag structure
struct multiboot2_tag {
    uint32_t type;
    uint32_t size;
};

// Multiboot 2 end tag
#define MULTIBOOT2_TAG_TYPE_END 0

// Multiboot 2 memory map tag
#define MULTIBOOT2_TAG_TYPE_MMAP 6

struct multiboot2_tag_mmap {
    struct multiboot2_tag tag;
    uint32_t entry_size;
    uint32_t entry_version;
    struct multiboot2_mmap_entry entries[];
} __attribute__((packed));

struct multiboot2_mmap_entry {
    uint64_t base_addr;
    uint64_t length;
    uint32_t type;
    uint32_t reserved;
} __attribute__((packed));

// Multiboot 2 bootloader name tag
#define MULTIBOOT2_TAG_TYPE_BOOT_LOADER_NAME 2

struct multiboot2_tag_string {
    struct multiboot2_tag tag;
    char string[];
} __attribute__((packed));

// Multiboot 2 kernel command line tag
#define MULTIBOOT2_TAG_TYPE_CMDLINE 1

// Function prototypes
const struct multiboot2_tag* find_multiboot2_tag(uint32_t type, const void* multiboot_info);
void print_memory_map(const void* multiboot_info);
void print_bootloader_info(const void* multiboot_info);
void print_kernel_cmdline(const void* multiboot_info);
void parse_multiboot2_info(const void* multiboot_info);

#endif // MULTIBOOT2_H
