global isr0
global isr1
global isr2
global isr3
global isr4
global isr5
global isr6
global isr7
global isr8
global isr9
global isr10
global isr11
global isr12
global isr13
global isr14
global isr15
global isr16
global isr17
global isr18
global isr19
global isr20
global isr21
global isr22
global isr23
global isr24
global isr25
global isr26
global isr27
global isr28
global isr29
global isr30
global isr31

section .text

; Exception ISRs
isr0:  irq_common_stub 0
isr1:  irq_common_stub 1
isr2:  irq_common_stub 2
isr3:  irq_common_stub 3
isr4:  irq_common_stub 4
isr5:  irq_common_stub 5
isr6:  irq_common_stub 6
isr7:  irq_common_stub 7
isr8:  irq_common_stub 8
isr9:  irq_common_stub 9
isr10: irq_common_stub 10
isr11: irq_common_stub 11
isr12: irq_common_stub 12
isr13: irq_common_stub 13
isr14: irq_common_stub 14
isr15: irq_common_stub 15
isr16: irq_common_stub 16
isr17: irq_common_stub 17
isr18: irq_common_stub 18
isr19: irq_common_stub 19
isr20: irq_common_stub 20
isr21: irq_common_stub 21
isr22: irq_common_stub 22
isr23: irq_common_stub 23
isr24: irq_common_stub 24
isr25: irq_common_stub 25
isr26: irq_common_stub 26
isr27: irq_common_stub 27
isr28: irq_common_stub 28
isr29: irq_common_stub 29
isr30: irq_common_stub 30
isr31: irq_common_stub 31

; IRQ Common Stub
%macro irq_common_stub 1
    extern irq_handler_%1
    push qword 0                 ; Error code (if none)
    push qword %1                ; ISR number
    jmp irq_handler_common
%endmacro

irq_handler_common:
    ; Save registers
    pushaq
    push ds
    push es
    push fs
    push gs

    ; Set up segments
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Call the C handler
    mov rdi, rsp
    call irq_handler

    ; Restore registers
    pop gs
    pop fs
    pop es
    pop ds
    popa
    add rsp, 16  ; Skip error code and ISR number
    iretq
