section .bss
align 16
gdt_ptr resb 10

section .data
align 16
gdt:
    dq 0x0000000000000000 ; Null Descriptor
    dq 0x00CF9A000000FFFF ; Code Segment Descriptor
    dq 0x00CF92000000FFFF ; Data Segment Descriptor
    dq 0x00CFFA000000FFFF ; User Code Segment Descriptor
    dq 0x00CFF2000000FFFF ; User Data Segment Descriptor

section .text
global gdt_flush
gdt_flush:
    ; Load GDT pointer into [gdt_ptr]
    lea rax, [gdt]
    mov word [gdt_ptr], 47      ; Limit (size of GDT - 1)
    mov qword [gdt_ptr + 2], rax ; Base address of the GDT

    ; Load the GDT
    lgdt [gdt_ptr]

    ; Reload segment registers
    mov ax, 0x10       ; Data segment selector (3rd entry in GDT)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Far jump to the new code segment to flush the instruction pipeline
    jmp 0x08:flush_done ; Code segment selector (2nd entry in GDT)

flush_done:
    ret
