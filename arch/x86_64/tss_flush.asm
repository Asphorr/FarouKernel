section .bss
align 16
tss resb 104 ; TSS structure, 104 bytes for x86_64 architecture
gdt_ptr resb 10

section .data
align 16
gdt:
    dq 0x0000000000000000     ; Null Descriptor
    dq 0x00CF9A000000FFFF     ; Kernel Code Segment Descriptor (0x08)
    dq 0x00CF92000000FFFF     ; Kernel Data Segment Descriptor (0x10)
    dq 0x00CFFA000000FFFF     ; User Code Segment Descriptor (0x18)
    dq 0x00CFF2000000FFFF     ; User Data Segment Descriptor (0x20)
    ; TSS Descriptor (0x28)
    dw 0x0067                 ; Limit (size of TSS - 1)
    dw tss                    ; Base address (lower 16 bits)
    db (tss >> 16) & 0xFF     ; Base address (middle 8 bits)
    db 0x89                   ; Access byte (present, DPL=0, executable, accessed)
    db 0x00                   ; Flags (granularity=0, 32-bit)
    db (tss >> 24) & 0xFF     ; Base address (upper 8 bits)
    dd (tss >> 32) & 0xFFFFFFFF ; Base address (upper 32 bits)
    dd 0x00000000             ; Reserved

section .text
global gdt_flush, tss_flush, setup_gdt, initialize_tss, setup_tss
extern isr_stub_table

gdt_flush:
    ; Load GDT pointer
    lea rax, [gdt]
    mov word [gdt_ptr], gdt_end - gdt - 1 ; Limit (size of GDT - 1)
    mov qword [gdt_ptr + 2], rax          ; Base address of the GDT

    ; Load the GDT using LGDT instruction
    lgdt [gdt_ptr]

    ; Load segment registers with Kernel Data Segment Selector (0x10)
    mov ax, 0x10       ; Kernel Data Segment Selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Far jump to the new Kernel Code Segment Selector (0x08)
    jmp 0x08:flush_done ; Kernel Code Segment Selector

flush_done:
    ret

tss_flush:
    ; Load the TSS segment selector (0x28)
    mov ax, 0x28
    ltr ax
    ret

initialize_tss:
    ; Initialize the TSS (Task State Segment)
    xor rax, rax
    mov qword [tss + 0x00], rax ; rsp0 (Ring 0 stack pointer)
    mov qword [tss + 0x08], rax ; rsp1 (Ring 1 stack pointer)
    mov qword [tss + 0x10], rax ; rsp2 (Ring 2 stack pointer)
    mov qword [tss + 0x18], rax ; ist1
    mov qword [tss + 0x20], rax ; ist2
    mov qword [tss + 0x28], rax ; ist3
    mov qword [tss + 0x30], rax ; ist4
    mov qword [tss + 0x38], rax ; ist5
    mov qword [tss + 0x40], rax ; ist6
    mov qword [tss + 0x48], rax ; ist7
    mov word [tss + 0x50], 0x00 ; Reserved
    ret

setup_tss:
    call initialize_tss
    call tss_flush
    ret

setup_gdt:
    call gdt_flush
    call setup_tss
    ret

gdt_end:
