section .bss
align 16
idt_ptr resb 10

section .data
align 16
idt:
    times 256 dq 0 ; Initialize 256 IDT entries with zeroes

section .text
global setup_idt, idt_flush, default_exception_handler, set_idt_entry
extern isr_stub_table

idt_flush:
    ; Load IDT pointer
    lea rax, [idt]
    mov word [idt_ptr], idt_end - idt - 1 ; Limit (size of IDT - 1)
    mov qword [idt_ptr + 2], rax          ; Base address of the IDT

    ; Load the IDT using LIDT instruction
    lidt [idt_ptr]
    ret

setup_idt:
    ; Initialize the IDT entries using ISR stubs
    lea rax, [isr_stub_table]
    mov rcx, 32 ; Number of ISRs to set up
    xor rbx, rbx ; Clear index

setup_idt_loop:
    cmp rbx, rcx
    jge setup_idt_done
    mov rdx, [rax + rbx * 8] ; Load the address of the ISR stub
    call set_idt_entry
    inc rbx
    jmp setup_idt_loop

setup_idt_done:
    call idt_flush
    ret

set_idt_entry:
    ; Input: rbx = Interrupt Vector, rdx = ISR Address
    ; Set an IDT entry with the provided ISR address and attributes
    mov rax, rdx
    mov word [idt + rbx * 16], ax             ; ISR lower 16 bits
    mov word [idt + rbx * 16 + 2], 0x08       ; Kernel Code Segment Selector
    mov byte [idt + rbx * 16 + 4], 0          ; Reserved
    mov byte [idt + rbx * 16 + 5], 0x8E       ; Present, Ring 0, 64-bit Interrupt Gate
    mov word [idt + rbx * 16 + 6], ax >> 16   ; ISR higher 16 bits
    mov dword [idt + rbx * 16 + 8], rdx >> 32 ; ISR highest 32 bits
    mov dword [idt + rbx * 16 + 12], 0        ; Reserved
    ret

default_exception_handler:
    ; Basic Exception Handler
    ; Replace this with specific handlers for various exceptions
    cli
    hlt
    ret

isr_stub_table:
    dq isr0, isr1, isr2, isr3, isr4, isr5, isr6, isr7
    dq isr8, isr9, isr10, isr11, isr12, isr13, isr14, isr15
    dq isr16, isr17, isr18, isr19, isr20, isr21, isr22, isr23
    dq isr24, isr25, isr26, isr27, isr28, isr29, isr30, isr31

%macro isr_stub 1
    global isr%1
    isr%1:
        push rax
        push rbx
        push rcx
        push rdx
        push rbp
        push rsi
        push rdi
        push r8
        push r9
        push r10
        push r11
        push r12
        push r13
        push r14
        push r15
        mov rdi, %1
        call default_exception_handler
        pop r15
        pop r14
        pop r13
        pop r12
        pop r11
        pop r10
        pop r9
        pop r8
        pop rdi
        pop rsi
        pop rbp
        pop rdx
        pop rcx
        pop rbx
        pop rax
        iretq
%endmacro

; Define ISR stubs for exceptions
isr_stub 0
isr_stub 1
isr_stub 2
isr_stub 3
isr_stub 4
isr_stub 5
isr_stub 6
isr_stub 7
isr_stub 8
isr_stub 9
isr_stub 10
isr_stub 11
isr_stub 12
isr_stub 13
isr_stub 14
isr_stub 15
isr_stub 16
isr_stub 17
isr_stub 18
isr_stub 19
isr_stub 20
isr_stub 21
isr_stub 22
isr_stub 23
isr_stub 24
isr_stub 25
isr_stub 26
isr_stub 27
isr_stub 28
isr_stub 29
isr_stub 30
isr_stub 31

idt_end:

; Example usage: Setup IDT
global setup_interrupts
setup_interrupts:
    call setup_idt
    ret

; Future Expansion Plan:
; 1. **Additional Exception Handlers**:
;    - Create specific handlers for each exception.
;    - For example, divide by zero (ISR 0), page fault (ISR 14), etc.
; 
; 2. **Interrupt Handlers for Devices**:
;    - Add handlers for hardware interrupts (IRQs) like timer (IRQ 0) and keyboard (IRQ 1).
;    - Map IRQs to ISRs using the Programmable Interrupt Controller (PIC).
; 
; 3. **System Call Handling**:
;    - Implement a system call mechanism using a specific ISR (e.g., ISR 0x80).
; 
; 4. **Advanced ISR Management**:
;    - Use macros to streamline the creation of ISR stubs.
;    - Group common ISRs to reduce redundancy.
; 
; 5. **Exception Handler Improvements**:
;    - Implement detailed error messages and fault diagnosis in exception handlers.
; 
; 6. **Enable Paging and Memory Management**:
;    - Ensure that memory management and paging structures support exception handlers.

; Example of more detailed exception handler:
global exception_handler_divide_by_zero
exception_handler_divide_by_zero:
    ; Handle divide by zero exception (ISR 0)
    ; Replace with appropriate handling logic
    cli
    hlt
    ret

%macro isr_stub_with_error_code 1
    global isr%1
    isr%1:
        push rax
        push rbx
        push rcx
        push rdx
        push rbp
        push rsi
        push rdi
        push r8
        push r9
        push r10
        push r11
        push r12
        push r13
        push r14
        push r15
        mov rdi, %1
        call default_exception_handler
        pop r15
        pop r14
        pop r13
        pop r12
        pop r11
        pop r10
        pop r9
        pop r8
        pop rdi
        pop rsi
        pop rbp
        pop rdx
        pop rcx
        pop rbx
        pop rax
        add rsp, 8 ; Error code is pushed automatically by the CPU
        iretq
%endmacro

; Error code ISR stubs (for page fault, etc.)
isr_stub_with_error_code 8 ; Double Fault
isr_stub_with_error_code 10 ; Invalid TSS
isr_stub_with_error_code 11 ; Segment Not Present
isr_stub_with_error_code 12 ; Stack-Segment Fault
isr_stub_with_error_code 13 ; General Protection Fault
isr_stub_with_error_code 14 ; Page Fault
