#ifndef _ARCH_H
#define _ARCH_H

#include <stdint.h>
#include <stdbool.h>

/* Architecture-specific definitions */

/* Instruction Set Architecture (ISA) /
typedef enum {
X86 = 1, / x86 family processors /
AMD64, / AMD64/x86-64 family processors /
ARMv7, / ARMv7 family processors /
ARMv8, / ARMv8 family processors /
RISCV, / RISC-V family processors */
} arch_isa_e;

extern const char *const arch_isa_names[];

/* Endianness */
typedef enum {
LITTLE_ENDIAN = 1,
BIG_ENDIAN,
} endianness_e;

/* Word size in bits */
typedef enum {
WORD_SIZE_32 = 1,
WORD_SIZE_64,
} word_size_e;

/* Address Space Layout Randomization (ASLR) /
typedef enum {
NO_ASLR = 1, / No ASLR /
FULL_ASLR, / Full ASLR /
PARTIAL_ASLR, / Partial ASLR */
} aslr_mode_e;

/* Stack growth direction /
typedef enum {
UPWARD_GROWTH = 1, / Grows upward from low addresses /
DOWNWARD_GROWTH, / Grows downward from high addresses */
} stack_growth_dir_e;

/* Page table entry count /
typedef enum {
SMALL_PAGE_TABLE = 1, / Small page table /
LARGE_PAGE_TABLE, / Large page table */
} page_table_entry_count_e;

/* Page table shift amount /
typedef enum {
SHIFT_AMOUNT_SMALL = 1, / Shift amount for small page tables /
SHIFT_AMOUNT_LARGE, / Shift amount for large page tables */
} page_table_shift_amount_e;

/* Virtual memory start address /
typedef enum {
VIRTUAL_ADDRESS_SPACE_BEGIN = 1, / Start of virtual address space /
VIRTUAL_ADDRESS_SPACE_END, / End of virtual address space */
} virtual_mem_addr_e;

/* Interrupt Controller Type /
typedef enum {
APIC = 1, / Advanced Programmable Interrupt Controller (APIC) /
IOAPIC, / Input/Output Advanced Programmable Interrupt Controller (IOAPIC) /
LOCAL_APIC, / Local Advanced Programmable Interrupt Controller (LAPIC) /
HPET, / High Precision Event Timer (HPET) */
} interrupt_controller_type_e;

/* Timekeeping frequency /
typedef enum {
CLOCK_TICKS_PER_SECOND = 1, / Number of clock ticks per second /
CLOCK_TICKS_PER_MILLISECOND, / Number of clock ticks per millisecond */
} timekeeping_freq_e;

/* Console Output Buffer Size /
typedef enum {
CONSOLE_OUTPUT_BUFFER_SIZE_FIXED = 1, / Fixed size console output buffer /
CONSOLE_OUTPUT_BUFFER_SIZE_VARIABLE, / Variable size console output buffer */
} console_outbuf_size_e;

/* Networking Enabled Flag /
typedef enum {
NETWORKING_DISABLED = 1, / Networking disabled /
NETWORKING_ENABLED, / Networking enabled */
} networking_flag_e;

/* File System Type /
typedef enum {
FAT32 = 1, / FAT32 file system /
EXT2, / ext2 file system /
EXT3, / ext3 file system /
EXT4, / ext4 file system /
REISERFS, / ReiserFS file system /
JFS, / Journaled File System (JFS) /
XFS, / XFS file system /
OCFS2, / Oracle Cluster Filesystem 2 (OCFS2) /
BTRFS, / Btrfs file system /
ZFS, / ZFS file system /
UDF, / Universal Disk Format (UDF) /
ISO9660, / ISO 9660 file system /
HPFS, / High Performance File System (HPFS) /
NTFS, / New Technology File System (NTFS) */
} fs_type_e;

/* Memory Management Enabled Flag /
typedef enum {
MEMORY_MANAGEMENT_DISABLED = 1, / Memory management disabled /
MEMORY_MANAGEMENT_ENABLED, / Memory management enabled */
} mem_manage_flag_e;

/* Kernel Debugging Enabled Flag /
typedef enum {
DEBUGGING_DISABLED = 1, / Kernel debugging disabled /
DEBUGGING_ENABLED, / Kernel debugging enabled */
} kernel_debug_flag_e;

/* Platform Specific Data Structures */
struct platform_specific_data {
union {
struct {
uint32_t foo;
uint64_t bar;
} x86;
struct {
uint32_t baz;
uint64_t quux;
} amd64;
struct {
uint32_t corge;
uint64_t grault;
} armv7;
struct {
uint32_t garply;
uint64_t waldo;
} armv8;
struct {
uint32_t fred;
uint64_t plugh;
