#include "multiboot2_parser.h"
#include <stdio.h>

// Multiboot2 header
__attribute__((section(".multiboot"), used))
const struct multiboot2_header multiboot2_header = {
    .magic = MULTIBOOT2_BOOTLOADER_MAGIC,
    .architecture = 0,
    .header_length = sizeof(struct multiboot2_header),
    .checksum = -(MULTIBOOT2_BOOTLOADER_MAGIC + 0 + sizeof(struct multiboot2_header))
};

// Function to find a Multiboot2 tag
const struct multiboot2_tag* find_multiboot2_tag(uint32_t type, const void* multiboot_info) {
    if (!multiboot_info) {
        LOG_ERROR("Invalid multiboot_info pointer");
        return NULL;
    }

    const struct multiboot2_info_header* mb_info = (const struct multiboot2_info_header*)multiboot_info;
    const struct multiboot2_tag* tag = (const struct multiboot2_tag*)((const uint8_t*)multiboot_info + sizeof(struct multiboot2_info_header));

    while (tag->type != MULTIBOOT2_TAG_TYPE_END) {
        if (tag->type == type) {
            return tag;
        }
        tag = (const struct multiboot2_tag*)ALIGN((uintptr_t)tag + tag->size, 8);
    }

    return NULL;
}

// Function to print memory map
void print_memory_map(const void* multiboot_info) {
    const struct multiboot2_tag* tag = find_multiboot2_tag(MULTIBOOT2_TAG_TYPE_MMAP, multiboot_info);
    if (!tag) {
        LOG_INFO("No memory map found");
        return;
    }

    const struct multiboot2_tag_mmap* mmap_tag = (const struct multiboot2_tag_mmap*)tag;
    const struct multiboot2_mmap_entry* entry = mmap_tag->entries;
    const struct multiboot2_mmap_entry* end = (const struct multiboot2_mmap_entry*)((const uint8_t*)mmap_tag + mmap_tag->size);

    LOG_INFO("Memory Map:");
    while (entry < end) {
        printf("  Base Address: 0x%016lx, Length: 0x%016lx, Type: %u\n",
               entry->base_addr, entry->length, entry->type);
        entry = (const struct multiboot2_mmap_entry*)((const uint8_t*)entry + mmap_tag->entry_size);
    }
}

// Function to print bootloader information
void print_bootloader_info(const void* multiboot_info) {
    const struct multiboot2_tag* tag = find_multiboot2_tag(MULTIBOOT2_TAG_TYPE_BOOT_LOADER_NAME, multiboot_info);
    if (tag) {
        const struct multiboot2_tag_string* bootloader_tag = (const struct multiboot2_tag_string*)tag;
        LOG_INFO("Bootloader: %s", bootloader_tag->string);
    } else {
        LOG_INFO("Bootloader information not available");
    }
}

// Function to print kernel command line
void print_kernel_cmdline(const void* multiboot_info) {
    const struct multiboot2_tag* tag = find_multiboot2_tag(MULTIBOOT2_TAG_TYPE_CMDLINE, multiboot_info);
    if (tag) {
        const struct multiboot2_tag_string* cmdline_tag = (const struct multiboot2_tag_string*)tag;
        LOG_INFO("Kernel Command Line: %s", cmdline_tag->string);
    } else {
        LOG_INFO("Kernel command line not available");
    }
}

// Function to handle and parse the Multiboot2 information
void parse_multiboot2_info(const void* multiboot_info) {
    if (!multiboot_info) {
        LOG_ERROR("Invalid Multiboot2 information pointer");
        return;
    }

    const struct multiboot2_info_header* mb_info = (const struct multiboot2_info_header*)multiboot_info;
    LOG_INFO("Multiboot2 Info Total Size: %u bytes", mb_info->total_size);

    print_memory_map(multiboot_info);
    print_bootloader_info(multiboot_info);
    print_kernel_cmdline(multiboot_info);

    // Additional parsing for other Multiboot2 tags can be added here
}

// Utility function to get human-readable memory type
static const char* get_memory_type_string(uint32_t type) {
    switch (type) {
        case 1: return "Available";
        case 2: return "Reserved";
        case 3: return "ACPI Reclaimable";
        case 4: return "NVS";
        case 5: return "Bad RAM";
        default: return "Unknown";
    }
}

// Enhanced memory map printing with human-readable types
void print_memory_map(const void* multiboot_info) {
    const struct multiboot2_tag* tag = find_multiboot2_tag(MULTIBOOT2_TAG_TYPE_MMAP, multiboot_info);
    if (!tag) {
        LOG_INFO("No memory map found");
        return;
    }

    const struct multiboot2_tag_mmap* mmap_tag = (const struct multiboot2_tag_mmap*)tag;
    const struct multiboot2_mmap_entry* entry = mmap_tag->entries;
    const struct multiboot2_mmap_entry* end = (const struct multiboot2_mmap_entry*)((const uint8_t*)mmap_tag + mmap_tag->size);

    LOG_INFO("Memory Map:");
    while (entry < end) {
        printf("  Base Address: 0x%016lx, Length: 0x%016lx, Type: %s\n",
               entry->base_addr, entry->length, get_memory_type_string(entry->type));
        entry = (const struct multiboot2_mmap_entry*)((const uint8_t*)entry + mmap_tag->entry_size);
    }
}

// Function to print system information
void print_system_info(const void* multiboot_info) {
    const struct multiboot2_tag* tag = find_multiboot2_tag(MULTIBOOT2_TAG_TYPE_BASIC_MEMINFO, multiboot_info);
    if (tag) {
        const struct multiboot2_tag_basic_meminfo* meminfo = (const struct multiboot2_tag_basic_meminfo*)tag;
        LOG_INFO("Lower memory: %u KB", meminfo->mem_lower);
        LOG_INFO("Upper memory: %u KB", meminfo->mem_upper);
    } else {
        LOG_INFO("Basic memory information not available");
    }

    // Add more system information parsing here (e.g., BIOS boot device, framebuffer info)
}

// Main entry point for Multiboot2 parsing
void multiboot2_entry(uint32_t magic, const void* multiboot_info) {
    if (magic != MULTIBOOT2_BOOTLOADER_MAGIC) {
        LOG_ERROR("Invalid Multiboot2 magic number: 0x%08x", magic);
        return;
    }

    LOG_INFO("Multiboot2 Magic: 0x%08x", magic);
    parse_multiboot2_info(multiboot_info);
    print_system_info(multiboot_info);
}
