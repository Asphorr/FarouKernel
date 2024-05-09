; boot.asm
bits 16
section .text
org 0x7C00

start:
    ; Setup stack
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

    ; Load kernel to 0x100000
    mov si, hello
    call print_string

    ; Load the kernel into memory
    mov ax, 0x1000
    mov es, ax
    mov di, 0x0000
    mov bx, 0x02
    mov ah, 0x02
    mov al, 10
    int 0x13

    ; Switch to 32-bit protected mode
    call switch_to_protected_mode

switch_to_protected_mode:
    ; Load GDT
    lgdt [gdt_descriptor]

    ; Enable A20 gate
    in al, 0x92
    or al, 2
    out 0x92, al

    ; Enable protected mode
    mov cr0, eax
    or eax, 0x1
    mov cr0, eax

    ; Jump to 32-bit code
    jmp 0x08:protected_mode_entry

bits 32
protected_mode_entry:
    ; Setup data segments
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Jump to 64-bit mode setup
    call switch_to_long_mode

switch_to_long_mode:
    ; Load LDT
    lgdt [gdt64_descriptor]

    ; Enable long mode
    mov eax, cr4
    or eax, 0x20
    mov cr4, eax

    mov ecx, 0xC0000080
    rdmsr
    or eax, 0x100
    wrmsr

    mov eax, cr0
    or eax, 0x80000001
    mov cr0, eax

    ; Jump to 64-bit mode
    jmp 0x28:long_mode_entry

bits 64
long_mode_entry:
    ; Setup data segments
    mov ax, 0x30
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Jump to kernel entry point
    extern kernel_main
    call kernel_main

print_string:
    pusha
    next_char:
        lodsb
        or al, al
        jz done
        mov ah, 0x0E
        int 0x10
        jmp next_char
    done:
    popa
    ret

hello db "Loading Kernel", 0

; GDT definitions
align 8
gdt:
    dq 0x0000000000000000        ; Null descriptor
    dq 0x00CF9A000000FFFF        ; Code 32-bit
    dq 0x00CF92000000FFFF        ; Data 32-bit
gdt_descriptor:
    dw gdt - gdt - 1
    dd gdt

align 8
gdt64:
    dq 0x0000000000000000        ; Null descriptor
    dq 0x00A09A000000FFFF        ; Code 64-bit
    dq 0x00A092000000FFFF        ; Data 64-bit
gdt64_descriptor:
    dw gdt64 - gdt64 - 1
    dd gdt64

times 510 - ($ - $$) db 0
dw 0xAA55
