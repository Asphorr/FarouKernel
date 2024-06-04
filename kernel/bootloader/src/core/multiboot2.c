#include "multiboot2.h"
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

// Multiboot 2 magic number and flags
#define MULTIBOOT2_BOOTLOADER_MAGIC 0x36d76289
#define MULTIBOOT2_FLAG 0x00000000

// Ensure the Multiboot 2 header is 8-byte aligned
#define ALIGN(x, a) (((x) + (a - 1)) & ~(a - 1))

// Multiboot 2 header fields
__attribute__((section(".multiboot"), used))
const struct multiboot2_header multiboot2_header = {
    .magic = MULTIBOOT2_BOOTLOADER_MAGIC,
    .architecture = 0,
    .header_length = sizeof(struct multiboot2_header),
    .checksum = -(MULTIBOOT2_BOOTLOADER_MAGIC + 0 + sizeof(struct multiboot2_header))
};

// Function to find a Multiboot 2 tag
const struct multiboot2_tag* find_multiboot2_tag(uint32_t type, const void* multiboot_info) {
    const struct multiboot2_tag* tag;
    const struct multiboot2_info_header* mb_info = multiboot_info;

    for (tag = (const struct multiboot2_tag*)((uint8_t*)multiboot_info + 8); tag->type != MULTIBOOT2_TAG_TYPE_END;
         tag = (const struct multiboot2_tag*)ALIGN((uintptr_t)tag + tag->size, 8)) {
        if (tag->type == type) {
            return tag;
        }
    }
    return NULL;
}

// Function to print memory map
void print_memory_map(const void* multiboot_info) {
    const struct multiboot2_tag_mmap* mmap_tag = (const struct multiboot2_tag_mmap*)find_multiboot2_tag(MULTIBOOT2_TAG_TYPE_MMAP, multiboot_info);

    if (!mmap_tag) {
        printf("No memory map found\n");
        return;
    }

    const struct multiboot2_mmap_entry* entry;
    printf("Memory Map:\n");
    for (entry = mmap_tag->entries; (uint8_t*)entry < (uint8_t*)mmap_tag + mmap_tag->size; entry = (const struct multiboot2_mmap_entry*)((uint8_t*)entry + mmap_tag->entry_size)) {
        printf("Base Address: 0x%016lx, Length: 0x%016lx, Type: %u\n",
               entry->base_addr, entry->length, entry->type);
    }
}

// Function to print bootloader information
void print_bootloader_info(const void* multiboot_info) {
    const struct multiboot2_tag_string* bootloader_tag = (const struct multiboot2_tag_string*)find_multiboot2_tag(MULTIBOOT2_TAG_TYPE_BOOT_LOADER_NAME, multiboot_info);

    if (bootloader_tag) {
        printf("Bootloader: %s\n", bootloader_tag->string);
    } else {
        printf("Bootloader information not available\n");
    }
}

void print_kernel_cmdline(const void* multiboot_info) {
    const struct multiboot2_tag_string* cmdline_tag = (const struct multiboot2_tag_string*)find_multiboot2_tag(MULTIBOOT2_TAG_TYPE_CMDLINE, multiboot_info);

    if (cmdline_tag) {
        printf("Kernel Command Line: %s\n", cmdline_tag->string);
    } else {
        printf("Kernel command line not available\n");
    }
}

// Function to handle and parse the Multiboot 2 information
void parse_multiboot2_info(const void* multiboot_info) {
    if (!multiboot_info) {
        printf("Invalid Multiboot 2 information pointer\n");
        return;
    }

    print_memory_map(multiboot_info);
    print_bootloader_info(multiboot_info);
    print_kernel_cmdline(multiboot_info);
}
