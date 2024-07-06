; boot.asm
bits 16
org 0x7C00

section .text
start:
    ; Setup stack
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti

    ; Print loading message
    mov si, msg_loading
    call print_string

    ; Load kernel
    mov ax, 0x1000
    mov es, ax
    xor bx, bx
    mov ah, 0x02
    mov al, 10
    mov ch, 0
    mov cl, 2
    mov dh, 0
    int 0x13

    ; Switch to protected mode
    cli
    lgdt [gdt_descriptor]
    mov eax, cr0
    or al, 1
    mov cr0, eax
    jmp CODE_SEG:protected_mode

bits 32
protected_mode:
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Switch to long mode
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    lgdt [gdt64_descriptor]
    jmp CODE_SEG64:long_mode

bits 64
long_mode:
    mov ax, DATA_SEG64
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Jump to kernel
    extern kernel_main
    call kernel_main
    jmp $

bits 16
print_string:
    lodsb
    test al, al
    jz .done
    mov ah, 0x0E
    int 0x10
    jmp print_string
.done:
    ret

section .data
msg_loading db "Loading Kernel...", 0

section .rodata
gdt:
    dq 0                    ; Null descriptor
.code: equ $ - gdt
    dq (1 << 43) | (1 << 44) | (1 << 47) | (1 << 53) ; Code segment
.data: equ $ - gdt
    dq (1 << 44) | (1 << 47) | (1 << 41)  ; Data segment
gdt_descriptor:
    dw $ - gdt - 1
    dd gdt

gdt64:
    dq 0                    ; Null descriptor
.code: equ $ - gdt64
    dq (1 << 43) | (1 << 44) | (1 << 47) | (1 << 53) ; 64-bit code segment
.data: equ $ - gdt64
    dq (1 << 44) | (1 << 47) | (1 << 41)  ; 64-bit data segment
gdt64_descriptor:
    dw $ - gdt64 - 1
    dd gdt64

CODE_SEG equ gdt.code
DATA_SEG equ gdt.data
CODE_SEG64 equ gdt64.code
DATA_SEG64 equ gdt64.data

times 510 - ($ - $$) db 0
dw 0xAA55
