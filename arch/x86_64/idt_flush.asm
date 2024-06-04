section .bss
align 16
idt_ptr resb 10

section .data
align 16
idt:
    times 256 dq 0 ; Initialize 256 IDT entries with zeroes

section .text
global setup_interrupts, idt_flush, set_idt_entry, setup_idt
global exception_handler_divide_by_zero, exception_handler_page_fault, exception_handler_general_protection_fault
global exception_handler_double_fault, exception_handler_invalid_tss, exception_handler_segment_not_present
global exception_handler_stack_segment_fault, exception_handler_machine_check, exception_handler_overflow
global irq0_handler, irq1_handler, syscall_handler
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
    mov rcx, 48 ; Number of ISRs + IRQs to set up
    xor rbx, rbx ; Clear index

setup_idt_loop:
    cmp rbx, rcx
    jge setup_idt_done
    mov rdx, [rax + rbx * 8] ; Load the address of the ISR stub
    call set_idt_entry
    inc rbx
    jmp setup_idt_loop

setup_idt_done:
    ; Set up system call ISR (example: 0x80)
    mov rbx, 0x80
    lea rdx, [syscall_handler]
    call set_idt_entry

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
    ; Basic Exception Handler for testing
    ; Replace this with specific handlers for each exception
    cli
    hlt
    ret

; Specific Exception Handlers
exception_handler_divide_by_zero:
    ; Handle divide by zero exception (ISR 0)
    ; Replace with appropriate handling logic
    mov rdi, "Divide by Zero Error"
    call exception_message
    cli
    hlt
    ret

exception_handler_page_fault:
    ; Handle page fault exception (ISR 14)
    ; Replace with appropriate handling logic
    mov rdi, "Page Fault Error"
    call exception_message
    cli
    hlt
    ret

exception_handler_general_protection_fault:
    ; Handle general protection fault exception (ISR 13)
    ; Replace with appropriate handling logic
    mov rdi, "General Protection Fault Error"
    call exception_message
    cli
    hlt
    ret

exception_handler_double_fault:
    ; Handle double fault exception (ISR 8)
    mov rdi, "Double Fault Error"
    call exception_message
    cli
    hlt
    ret

exception_handler_invalid_tss:
    ; Handle invalid TSS exception (ISR 10)
    mov rdi, "Invalid TSS Error"
    call exception_message
    cli
    hlt
    ret

exception_handler_segment_not_present:
    ; Handle segment not present exception (ISR 11)
    mov rdi, "Segment Not Present Error"
    call exception_message
    cli
    hlt
    ret

exception_handler_stack_segment_fault:
    ; Handle stack-segment fault exception (ISR 12)
    mov rdi, "Stack Segment Fault Error"
    call exception_message
    cli
    hlt
    ret

exception_handler_machine_check:
    ; Handle machine check exception (ISR 18)
    mov rdi, "Machine Check Error"
    call exception_message
    cli
    hlt
    ret

exception_handler_overflow:
    ; Handle overflow exception (ISR 4)
    mov rdi, "Overflow Error"
    call exception_message
    cli
    hlt
    ret

irq0_handler:
    ; Handle timer interrupt (IRQ 0)
    ; Replace with appropriate handling logic
    mov rdi, "Timer Interrupt (IRQ 0)"
    call irq_message
    ; Acknowledge the interrupt by sending EOI to the PIC
    mov al, 0x20
    out 0x20, al
    ret

irq1_handler:
    ; Handle keyboard interrupt (IRQ 1)
    ; Replace with appropriate handling logic
    mov rdi, "Keyboard Interrupt (IRQ 1)"
    call irq_message
    ; Acknowledge the interrupt by sending EOI to the PIC
    mov al, 0x20
    out 0x20, al
    ret

irq2_handler:
    ; Handle cascade interrupt (IRQ 2)
    mov rdi, "Cascade Interrupt (IRQ 2)"
    call irq_message
    ; Acknowledge the interrupt by sending EOI to the PIC
    mov al, 0x20
    out 0x20, al
    ret

irq3_handler:
    ; Handle COM2 interrupt (IRQ 3)
    mov rdi, "COM2 Interrupt (IRQ 3)"
    call irq_message
    mov al, 0x20
    out 0x20, al
    ret

irq4_handler:
    ; Handle COM1 interrupt (IRQ 4)
    mov rdi, "COM1 Interrupt (IRQ 4)"
    call irq_message
    mov al, 0x20
    out 0x20, al
    ret

irq5_handler:
    ; Handle LPT2 interrupt (IRQ 5)
    mov rdi, "LPT2 Interrupt (IRQ 5)"
    call irq_message
    mov al, 0x20
    out 0x20, al
    ret

irq6_handler:
    ; Handle floppy disk interrupt (IRQ 6)
    mov rdi, "Floppy Disk Interrupt (IRQ 6)"
    call irq_message
    mov al, 0x20
    out 0x20, al
    ret

irq7_handler:
    ; Handle LPT1/Spurious interrupt (IRQ 7)
    mov rdi, "LPT1/Spurious Interrupt (IRQ 7)"
    call irq_message
    mov al, 0x20
    out 0x20, al
    ret

irq8_handler:
    ; Handle CMOS real-time clock interrupt (IRQ 8)
    mov rdi, "CMOS Real-Time Clock Interrupt (IRQ 8)"
    call irq_message
    ; Acknowledge the interrupt by sending EOI to the PIC
    mov al, 0x20
    out 0xA0, al
    out 0x20, al
    ret

irq9_handler:
    ; Handle free IRQ 9
    mov rdi, "Free IRQ (IRQ 9)"
    call irq_message
    ; Acknowledge the interrupt by sending EOI to the PIC
    mov al, 0x20
    out 0xA0, al
    out 0x20, al
    ret

irq10_handler:
    ; Handle free IRQ 10
    mov rdi, "Free IRQ (IRQ 10)"
    call irq_message
    ; Acknowledge the interrupt by sending EOI to the PIC
    mov al, 0x20
    out 0xA0, al
    out 0x20, al
    ret

irq11_handler:
    ; Handle free IRQ 11
    mov rdi, "Free IRQ (IRQ 11)"
    call irq_message
    ; Acknowledge the interrupt by sending EOI to the PIC
    mov al, 0x20
    out 0xA0, al
    out 0x20, al
    ret

irq12_handler:
    ; Handle PS/2 mouse interrupt (IRQ 12)
    mov rdi, "PS/2 Mouse Interrupt (IRQ 12)"
    call irq_message
    ; Acknowledge the interrupt by sending EOI to the PIC
    mov al, 0x20
    out 0xA0, al
    out 0x20, al
    ret

irq13_handler:
    ; Handle FPU interrupt (IRQ 13)
    mov rdi, "FPU Interrupt (IRQ 13)"
    call irq_message
    ; Acknowledge the interrupt by sending EOI to the PIC
    mov al, 0x20
    out 0xA0, al
    out 0x20, al
    ret

irq14_handler:
    ; Handle primary ATA hard disk interrupt (IRQ 14)
    mov rdi, "Primary ATA Hard Disk Interrupt (IRQ 14)"
    call irq_message
    ; Acknowledge the interrupt by sending EOI to the PIC
    mov al, 0x20
    out 0xA0, al
    out 0x20, al
    ret

irq15_handler:
    ; Handle secondary ATA hard disk interrupt (IRQ 15)
    mov rdi, "Secondary ATA Hard Disk Interrupt (IRQ 15)"
    call irq_message
    ; Acknowledge the interrupt by sending EOI to the PIC
    mov al, 0x20
    out 0xA0, al
    out 0x20, al
    ret

syscall_handler:
    ; Handle system call (ISR 0x80)
    ; Replace with appropriate system call handling logic
    mov rdi, "System Call (ISR 0x80)"
    call syscall_message
    ; Add your system call handling logic here
    ret

; Functions to print error or IRQ messages (replace with your own implementation)
section .text
global exception_message, irq_message, syscall_message

exception_message:
    ; Input: rdi = error message
    ; Replace with your own message printing logic
    ; Example: print the error message
    ret

irq_message:
    ; Input: rdi = IRQ message
    ; Replace with your own message printing logic
    ; Example: print the IRQ message
    ret

syscall_message:
    ; Input: rdi = syscall message
    ; Replace with your own message printing logic
    ; Example: print the syscall message
    ret

; Define ISR stubs for exceptions and IRQs
isr_stub_table:
    dq isr0, isr1, isr2, isr3, isr4, isr5, isr6, isr7
    dq isr8, isr9, isr10, isr11, isr12, isr13, isr14, isr15
    dq isr16, isr17, isr18, isr19, isr20, isr21, isr22, isr23
    dq isr24, isr25, isr26, isr27, isr28, isr29, isr30, isr31
    dq irq0_handler, irq1_handler, irq2_handler, irq3_handler, irq4_handler, irq5_handler, irq6_handler, irq7_handler
    dq irq8_handler, irq9_handler, irq10_handler, irq11_handler, irq12_handler, irq13_handler, irq14_handler, irq15_handler

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

; Define ISR stubs for exceptions
isr_stub 0  ; Divide by Zero
isr_stub 1  ; Debug
isr_stub 2  ; Non-maskable Interrupt
isr_stub 3  ; Breakpoint
isr_stub 4  ; Overflow
isr_stub 5  ; Bound Range Exceeded
isr_stub 6  ; Invalid Opcode
isr_stub 7  ; Device Not Available
isr_stub_with_error_code 8  ; Double Fault
isr_stub 9  ; Coprocessor Segment Overrun (Not used)
isr_stub_with_error_code 10 ; Invalid TSS
isr_stub_with_error_code 11 ; Segment Not Present
isr_stub_with_error_code 12 ; Stack-Segment Fault
isr_stub_with_error_code 13 ; General Protection Fault
isr_stub_with_error_code 14 ; Page Fault
isr_stub 16 ; x87 Floating-Point Exception
isr_stub_with_error_code 17 ; Alignment Check
isr_stub 18 ; Machine Check
isr_stub 19 ; SIMD Floating-Point Exception
isr_stub 20 ; Virtualization Exception
isr_stub 30 ; Security Exception

; Define IRQ stubs for hardware interrupts
isr_stub 32 ; IRQ 0 Timer
isr_stub 33 ; IRQ 1 Keyboard
isr_stub 34 ; IRQ 2 Cascade
isr_stub 35 ; IRQ 3 COM2
isr_stub 36 ; IRQ 4 COM1
isr_stub 37 ; IRQ 5 LPT2
isr_stub 38 ; IRQ 6 Floppy Disk
isr_stub 39 ; IRQ 7 LPT1
isr_stub 40 ; IRQ 8 CMOS Real-Time Clock
isr_stub 41 ; IRQ 9 Free for peripherals
isr_stub 42 ; IRQ 10 Free for peripherals
isr_stub 43 ; IRQ 11 Free for peripherals
isr_stub 44 ; IRQ 12 PS/2 Mouse
isr_stub 45 ; IRQ 13 FPU
isr_stub 46 ; IRQ 14 Primary ATA Hard Disk
isr_stub 47 ; IRQ 15 Secondary ATA Hard Disk

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
    mov rdi, "Divide by Zero Error"
    call exception_message
    cli
    hlt
    ret

exception_handler_page_fault:
    ; Handle page fault exception (ISR 14)
    ; Replace with appropriate handling logic
    mov rdi, "Page Fault Error"
    call exception_message
    cli
    hlt
    ret

exception_handler_general_protection_fault:
    ; Handle general protection fault exception (ISR 13)
    ; Replace with appropriate handling logic
    mov rdi, "General Protection Fault Error"
    call exception_message
    cli
    hlt
    ret

exception_handler_double_fault:
    ; Handle double fault exception (ISR 8)
    ; Replace with appropriate handling logic
    mov rdi, "Double Fault Error"
    call exception_message
    cli
    hlt
    ret

exception_handler_invalid_tss:
    ; Handle invalid TSS exception (ISR 10)
    ; Replace with appropriate handling logic
    mov rdi, "Invalid TSS Error"
    call exception_message
    cli
    hlt
    ret

exception_handler_segment_not_present:
    ; Handle segment not present exception (ISR 11)
    ; Replace with appropriate handling logic
    mov rdi, "Segment Not Present Error"
    call exception_message
    cli
    hlt
    ret

exception_handler_stack_segment_fault:
    ; Handle stack-segment fault exception (ISR 12)
    ; Replace with appropriate handling logic
    mov rdi, "Stack Segment Fault Error"
    call exception_message
    cli
    hlt
    ret

exception_handler_machine_check:
    ; Handle machine check exception (ISR 18)
    ; Replace with appropriate handling logic
    mov rdi, "Machine Check Error"
    call exception_message
    cli
    hlt
    ret

exception_handler_overflow:
    ; Handle overflow exception (ISR 4)
    ; Replace with appropriate handling logic
    mov rdi, "Overflow Error"
    call exception_message
    cli
    hlt
    ret

; Utility functions for printing error and IRQ messages
global exception_message, irq_message, syscall_message

exception_message:
    ; Input: rdi = error message
    ; Replace with your own message printing logic
    ; Example: print the error message
    ; Placeholder for printing (replace with your actual print function)
    ret

irq_message:
    ; Input: rdi = IRQ message
    ; Replace with your own message printing logic
    ; Example: print the IRQ message
    ; Placeholder for printing (replace with your actual print function)
    ret

syscall_message:
    ; Input: rdi = syscall message
    ; Replace with your own message printing logic
    ; Example: print the syscall message
    ; Placeholder for printing (replace with your actual print function)
    ret
