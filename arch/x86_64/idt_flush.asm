; Constants
IDT_SIZE equ 4096
IDT_ENTRY_SIZE equ 16

section .bss
align 16
idt_ptr resb 10
idt resb IDT_SIZE

section .data
align 16
; Messages for specific exceptions (example)
divide_by_zero_msg: db "Divide by Zero Error", 0
page_fault_msg: db "Page Fault Error", 0
general_protection_msg: db "General Protection Fault Error", 0
overflow_msg: db "Overflow Error", 0
double_fault_msg: db "Double Fault Error", 0
invalid_tss_msg: db "Invalid TSS Error", 0
segment_not_present_msg: db "Segment Not Present Error", 0
stack_segment fault_msg: db "Stack Segment Fault Error", 0
machine_check_msg: db "Machine Check Error", 0

; Messages for IRQs (example)
irq0_msg: db "Timer Interrupt (IRQ 0)", 0
irq1_msg: db "Keyboard Interrupt (IRQ 1)", 0
irq2_msg: db "Cascade Interrupt (IRQ 2)", 0
irq3_msg: db "COM2 Interrupt (IRQ 3)", 0
irq4_msg: db "COM1 Interrupt (IRQ 4)", 0
irq5_msg: db "LPT2 Interrupt (IRQ 5)", 0
irq6_msg: db "Floppy Disk Interrupt (IRQ 6)", 0
irq7_msg: db "LPT1/Spurious Interrupt (IRQ 7)", 0
irq8_msg: db "CMOS Real-Time Clock Interrupt (IRQ 8)", 0
irq9_msg: db "Free IRQ (IRQ 9)", 0
irq10_msg: db "Free IRQ (IRQ 10)", 0
irq11_msg: db "Free IRQ (IRQ 11)", 0
irq12_msg: db "PS/2 Mouse Interrupt (IRQ 12)", 0
irq13_msg: db "FPU Interrupt (IRQ 13)", 0
irq14_msg: db "Primary ATA Hard Disk Interrupt (IRQ 14)", 0
irq15_msg: db "Secondary ATA Hard Disk Interrupt (IRQ 15)", 0

; Message for syscall
syscall_msg: db "System Call (ISR 0x80)", 0
unknown_syscall_msg: db "Unknown System Call", 0

section .text
global setup_interrupts, idt_flush, set_idt_entry, setup_idt
global exception_handler_divide_by_zero, exception_handler_page_fault, exception_handler_general_protection_fault
global exception_handler_double_fault, exception_handler_invalid_tss, exception_handler_segment_not_present
global exception_handler_stack_segment_fault, exception_handler_machine_check, exception_handler_overflow
global irq0_handler, irq1_handler, irq2_handler, irq3_handler, irq4_handler, irq5_handler, irq6_handler, irq7_handler
global irq8_handler, irq9_handler, irq10_handler, irq11_handler, irq12_handler, irq13_handler, irq14_handler, irq15_handler
global syscall_handler

extern default_exception_handler

; Function to load the IDT
idt_flush:
    lea rax, [idt]
    mov word [idt_ptr], IDT_SIZE - 1
    mov qword [idt_ptr + 2], rax
    lidt [idt_ptr]
    ret

; Function to set up the IDT
setup_idt:
    ; Initialize all entries with default handler
    mov rcx, 256
    xor rbx, rbx
    lea rdx, [default_exception_handler]
init_loop:
    call set_idt_entry
    inc rbx
    loop init_loop

    ; Set specific exception handlers
    mov rbx, 0
    lea rdx, [exception_handler_divide_by_zero]
    call set_idt_entry
    mov rbx, 4
    lea rdx, [exception_handler_overflow]
    call set_idt_entry
    mov rbx, 8
    lea rdx, [exception_handler_double_fault]
    call set_idt_entry
    mov rbx, 10
    lea rdx, [exception_handler_invalid_tss]
    call set_idt_entry
    mov rbx, 11
    lea rdx, [exception_handler_segment_not_present]
    call set_idt_entry
    mov rbx, 12
    lea rdx, [exception_handler_stack_segment_fault]
    call set_idt_entry
    mov rbx, 13
    lea rdx, [exception_handler_general_protection_fault]
    call set_idt_entry
    mov rbx, 14
    lea rdx, [exception_handler_page_fault]
    call set_idt_entry
    mov rbx, 18
    lea rdx, [exception_handler_machine_check]
    call set_idt_entry

    ; Set IRQ handlers (vectors 32-47)
    mov rbx, 32
    lea rdx, [irq0_handler]
    call set_idt_entry
    mov rbx, 33
    lea rdx, [irq1_handler]
    call set_idt_entry
    mov rbx, 34
    lea rdx, [irq2_handler]
    call set_idt_entry
    mov rbx, 35
    lea rdx, [irq3_handler]
    call set_idt_entry
    mov rbx, 36
    lea rdx, [irq4_handler]
    call set_idt_entry
    mov rbx, 37
    lea rdx, [irq5_handler]
    call set_idt_entry
    mov rbx, 38
    lea rdx, [irq6_handler]
    call set_idt_entry
    mov rbx, 39
    lea rdx, [irq7_handler]
    call set_idt_entry
    mov rbx, 40
    lea rdx, [irq8_handler]
    call set_idt_entry
    mov rbx, 41
    lea rdx, [irq9_handler]
    call set_idt_entry
    mov rbx, 42
    lea rdx, [irq10_handler]
    call set_idt_entry
    mov rbx, 43
    lea rdx, [irq11_handler]
    call set_idt_entry
    mov rbx, 44
    lea rdx, [irq12_handler]
    call set_idt_entry
    mov rbx, 45
    lea rdx, [irq13_handler]
    call set_idt_entry
    mov rbx, 46
    lea rdx, [irq14_handler]
    call set_idt_entry
    mov rbx, 47
    lea rdx, [irq15_handler]
    call set_idt_entry

    ; Set syscall handler at vector 0x80
    mov rbx, 0x80
    lea rdx, [syscall_handler]
    call set_idt_entry

    call idt_flush
    ret

; Function to set an IDT entry
set_idt_entry:
    ; Input: rbx = Interrupt Vector, rdx = ISR Address
    mov rax, rdx
    mov word [idt + rbx * IDT_ENTRY_SIZE], ax          ; ISR offset 0-15
    shr rax, 16
    mov word [idt + rbx * IDT_ENTRY_SIZE + 6], ax      ; ISR offset 16-31
    shr rax, 16
    mov dword [idt + rbx * IDT_ENTRY_SIZE + 8], eax    ; ISR offset 32-63
    mov word [idt + rbx * IDT_ENTRY_SIZE + 2], 0x08    ; Kernel Code Segment Selector
    mov byte [idt + rbx * IDT_ENTRY_SIZE + 4], 0       ; IST (no stack switch)
    mov byte [idt + rbx * IDT_ENTRY_SIZE + 5], 0x8E    ; Present, Ring 0, Interrupt Gate
    mov dword [idt + rbx * IDT_ENTRY_SIZE + 12], 0     ; Reserved
    ret

; Default exception handler
default_exception_handler:
    ; Basic handler for unhandled exceptions
    cli
    hlt
    ret

; Specific exception handlers
exception_handler_divide_by_zero:
    mov rdi, divide_by_zero_msg
    call exception_message
    cli
    hlt
    ret

exception_handler_overflow:
    mov rdi, overflow_msg
    call exception_message
    cli
    hlt
    ret

exception_handler_double_fault:
    mov rdi, double_fault_msg
    call exception_message
    cli
    hlt
    ret

exception_handler_invalid_tss:
    mov rdi, invalid_tss_msg
    call exception_message
    cli
    hlt
    ret

exception_handler_segment_not_present:
    mov rdi, segment_not_present_msg
    call exception_message
    cli
    hlt
    ret

exception_handler_stack_segment_fault:
    mov rdi, stack_segment_fault_msg
    call exception_message
    cli
    hlt
    ret

exception_handler_general_protection_fault:
    mov rdi, general_protection_msg
    call exception_message
    cli
    hlt
    ret

exception_handler_page_fault:
    mov rdi, page_fault_msg
    call exception_message
    cli
    hlt
    ret

exception_handler_machine_check:
    mov rdi, machine_check_msg
    call exception_message
    cli
    hlt
    ret

; IRQ handlers
irq0_handler:
    mov rdi, irq0_msg
    call irq_message
    mov al, 0x20
    out 0x20, al
    ret

irq1_handler:
    mov rdi, irq1_msg
    call irq_message
    mov al, 0x20
    out 0x20, al
    ret

irq2_handler:
    mov rdi, irq2_msg
    call irq_message
    mov al, 0x20
    out 0x20, al
    ret

irq3_handler:
    mov rdi, irq3_msg
    call irq_message
    mov al, 0x20
    out 0x20, al
    ret

irq4_handler:
    mov rdi, irq4_msg
    call irq_message
    mov al, 0x20
    out 0x20, al
    ret

irq5_handler:
    mov rdi, irq5_msg
    call irq_message
    mov al, 0x20
    out 0x20, al
    ret

irq6_handler:
    mov rdi, irq6_msg
    call irq_message
    mov al, 0x20
    out 0x20, al
    ret

irq7_handler:
    mov rdi, irq7_msg
    call irq_message
    mov al, 0x20
    out 0x20, al
    ret

irq8_handler:
    mov rdi, irq8_msg
    call irq_message
    mov al, 0x20
    out 0xA0, al
    out 0x20, al
    ret

irq9_handler:
    mov rdi, irq9_msg
    call irq_message
    mov al, 0x20
    out 0xA0, al
    out 0x20, al
    ret

irq10_handler:
    mov rdi, irq10_msg
    call irq_message
    mov al, 0x20
    out 0xA0, al
    out 0x20, al
    ret

irq11_handler:
    mov rdi, irq11_msg
    call irq_message
    mov al, 0x20
    out 0xA0, al
    out 0x20, al
    ret

irq12_handler:
    mov rdi, irq12_msg
    call irq_message
    mov al, 0x20
    out 0xA0, al
    out 0x20, al
    ret

irq13_handler:
    mov rdi, irq13_msg
    call irq_message
    mov al, 0x20
    out 0xA0, al
    out 0x20, al
    ret

irq14_handler:
    mov rdi, irq14_msg
    call irq_message
    mov al, 0x20
    out 0xA0, al
    out 0x20, al
    ret

irq15_handler:
    mov rdi, irq15_msg
    call irq_message
    mov al, 0x20
    out 0xA0, al
    out 0x20, al
    ret

; Syscall handler
syscall_handler:
    push rbp
    mov rbp, rsp
    cmp rax, 0      ; Example syscall number in rax
    je syscall_print
    mov rdi, unknown_syscall_msg
    call syscall_message
syscall_exit:
    pop rbp
    iretq           ; Use iretq for proper return

syscall_print:
    call syscall_message
    jmp syscall_exit

; Utility functions for printing messages (example VGA text mode)
exception_message:
    push rax
    push rbx
    mov rbx, 0xB8000
    mov ah, 0x0F    ; White on black
print_loop:
    mov al, [rdi]
    test al, al
    jz print_done
    mov [rbx], ax
    inc rdi
    add rbx, 2
    jmp print_loop
print_done:
    pop rbx
    pop rax
    ret

irq_message:
    ; Similar to exception_message
    jmp exception_message

syscall_message:
    ; Similar to exception_message
    jmp exception_message

; Initialize PIC (example for legacy PIC)
init_pic:
    push rax
    ; ICW1: Initialize PICs
    mov al, 0x11
    out 0x20, al  ; Master PIC
    out 0xA0, al  ; Slave PIC
    ; ICW2: Set vector offsets
    mov al, 0x20  ; Master PIC: IRQs 0-7 to 32-39
    out 0x21, al
    mov al, 0x28  ; Slave PIC: IRQs 8-15 to 40-47
    out 0xA1, al
    ; ICW3: Master-Slave wiring
    mov al, 0x04  ; Master has slave at IRQ 2
    out 0x21, al
    mov al, 0x02  ; Slave ID 2
    out 0xA1, al
    ; ICW4: 8086 mode
    mov al, 0x01
    out 0x21, al
    out 0xA1, al
    ; Mask all interrupts initially
    mov al, 0xFF
    out 0x21, al
    out 0xA1, al
    pop rax
    ret

; Setup interrupts (including PIC initialization)
setup_interrupts:
    call init_pic
    call setup_idt
    ret
