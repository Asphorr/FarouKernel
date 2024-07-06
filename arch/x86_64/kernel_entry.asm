section .text
global _start
extern kernel_init

_start:
    ; Disable interrupts
    cli

    ; Set up stack
    mov rsp, stack_top

    ; Set up segment registers
    xor eax, eax
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Clear direction flag
    cld

    ; Check multiboot signature if needed
    ; (Uncomment and implement if you're using multiboot)
    ; cmp eax, 0x36D76289
    ; jne .no_multiboot
    ; mov [multiboot_info], ebx

    ; Clear BSS section
    call clear_bss

    ; Initialize essential CPU features
    call cpu_init

    ; Call kernel initialization function
    call kernel_init

    ; Enable interrupts
    sti

    ; Halt indefinitely
.halt:
    hlt
    jmp .halt

; .no_multiboot:
;     ; Handle no multiboot case
;     mov esi, no_multiboot_msg
;     call print_error
;     jmp .halt

clear_bss:
    extern bss_start, bss_end
    mov rdi, bss_start
    mov rcx, bss_end
    sub rcx, rdi
    xor eax, eax
    rep stosb
    ret

cpu_init:
    ; Enable SSE and AVX if available
    mov rax, cr0
    and ax, 0xFFFB  ; clear coprocessor emulation CR0.EM
    or ax, 0x2      ; set coprocessor monitoring  CR0.MP
    mov cr0, rax
    mov rax, cr4
    or ax, 3 << 9   ; set CR4.OSFXSR and CR4.OSXMMEXCPT
    mov cr4, rax

    ; More CPU initializations can be added here

    ret

section .bss
align 16
stack_bottom:
    resb 16384 ; 16 KiB stack
stack_top:

; section .data
; no_multiboot_msg db "Error: No Multiboot-compliant bootloader detected", 0

; section .bss
; multiboot_info: resq 1
