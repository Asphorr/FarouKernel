section .text
global _start
extern kernel_init

_start:
    ; Set up stack
    mov rsp, _stack_start

    ; Set up segment registers
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Call kernel initialization function
    call kernel_init

    ; Halt indefinitely
halt:
    hlt
    jmp halt

section .bss
_stack_start resb 4096
