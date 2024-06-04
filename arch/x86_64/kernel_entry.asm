section .text
global _start
extern kernel_init

_start:
    ; Set up stack
    mov rsp, _stack_start

    ; Call kernel initialization function
    call kernel_init

    ; Halt indefinitely
halt:
    hlt
    jmp halt
