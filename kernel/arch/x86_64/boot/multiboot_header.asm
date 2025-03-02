; multiboot_header.asm - Enhanced Multiboot2 Header
; This implementation follows the Multiboot2 Specification (https://www.gnu.org/software/grub/manual/multiboot2/)
; with optimizations for clarity, maintainability, and extensibility

; Define Multiboot2 constants for improved readability
%define MULTIBOOT2_MAGIC         0xe85250d6
%define MULTIBOOT2_ARCHITECTURE  0         ; Protected mode i386
%define MULTIBOOT2_HEADER_TAG_END 0
%define MULTIBOOT2_HEADER_TAG_OPTIONAL 1

; Alignment for performance optimization
section .multiboot_header
align 8

header_start:
    ; Mandatory Multiboot2 header fields
    dd MULTIBOOT2_MAGIC
    dd MULTIBOOT2_ARCHITECTURE
    dd header_end - header_start       ; Header size
    dd -(MULTIBOOT2_MAGIC + MULTIBOOT2_ARCHITECTURE + (header_end - header_start)) ; Checksum (improved readability)

    ; ===== Optional Multiboot Tags =====
    ; Information request tag - request specific information from bootloader
    align 8
    dw 1        ; Type: Information request
    dw 0        ; Flags
    dd 16       ; Size
    dd 1        ; Request memory map
    dd 2        ; Request boot device
    dd 5        ; Request ACPI old RSDP
    
    ; Address tag - specify preferred load address
    align 8
    dw 2        ; Type: Address
    dw 0        ; Flags
    dd 24       ; Size
    dd 0x00100000 ; Header load address
    dd 0x00100000 ; Load address
    dd 0        ; Load end address (0 = no preference)
    dd 0        ; BSS end address (0 = no preference)
    
    ; Framebuffer tag - request specific framebuffer parameters
    align 8
    dw 5        ; Type: Framebuffer
    dw 0        ; Flags
    dd 20       ; Size
    dd 1024     ; Width
    dd 768      ; Height
    dd 32       ; Depth (bits per pixel)
    
    ; Module alignment tag - ensure modules are aligned
    align 8
    dw 6        ; Type: Module alignment
    dw 0        ; Flags
    dd 8        ; Size
    
    ; ===== End Tag (Required) =====
    align 8
    dw MULTIBOOT2_HEADER_TAG_END 
    dw 0        ; Flags
    dd 8        ; Size
header_end:
