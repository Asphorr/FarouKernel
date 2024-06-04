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

global irq_handler_common

section .text

; Generate all ISRs using a macro
%macro define_isr 1
global isr%1
isr%1:
    irq_common_stub %1
%endmacro

; Define all ISRs
define_isr 0
define_isr 1
define_isr 2
define_isr 3
define_isr 4
define_isr 5
define_isr 6
define_isr 7
define_isr 8
define_isr 9
define_isr 10
define_isr 11
define_isr 12
define_isr 13
define_isr 14
define_isr 15
define_isr 16
define_isr 17
define_isr 18
define_isr 19
define_isr 20
define_isr 21
define_isr 22
define_isr 23
define_isr 24
define_isr 25
define_isr 26
define_isr 27
define_isr 28
define_isr 29
define_isr 30
define_isr 31

; IRQ Common Stub
%macro irq_common_stub 1
    extern irq_handler_%1
    push qword 0                 ; Error code (if none)
    push qword %1                ; ISR number
    jmp irq_handler_common
%endmacro

; General-purpose IRQ handler, calls a C function
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
    mov rdi, rsp                 ; Pass stack pointer to C handler
    call irq_handler

    ; Restore registers
    pop gs
    pop fs
    pop es
    pop ds
    popaq
    add rsp, 16                  ; Skip error code and ISR number
    iretq

; External C Function Declaration
extern irq_handler

; Example IRQ Handler Declarations
extern irq_handler_0
extern irq_handler_1
extern irq_handler_2
extern irq_handler_3
extern irq_handler_4
extern irq_handler_5
extern irq_handler_6
extern irq_handler_7
extern irq_handler_8
extern irq_handler_9
extern irq_handler_10
extern irq_handler_11
extern irq_handler_12
extern irq_handler_13
extern irq_handler_14
extern irq_handler_15
extern irq_handler_16
extern irq_handler_17
extern irq_handler_18
extern irq_handler_19
extern irq_handler_20
extern irq_handler_21
extern irq_handler_22
extern irq_handler_23
extern irq_handler_24
extern irq_handler_25
extern irq_handler_26
extern irq_handler_27
extern irq_handler_28
extern irq_handler_29
extern irq_handler_30
extern irq_handler_31
