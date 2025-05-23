section .bss
align 16
gdt_ptr: resq 1
         resw 1
tss:     resb 104 ; TSS structure, 104 bytes as per x86_64 architecture

section .data
align 16
gdt:
    dq 0x0000000000000000     ; Null Descriptor
    dq 0x00AF9A000000FFFF     ; Kernel Code Segment Descriptor (0x08)
    dq 0x00AF92000000FFFF     ; Kernel Data Segment Descriptor (0x10)
    dq 0x00AFFA000000FFFF     ; User Code Segment Descriptor (0x18)
    dq 0x00AFF2000000FFFF     ; User Data Segment Descriptor (0x20)
    ; TSS Descriptor (0x28)
    dw 104                    ; Limit (size of TSS - 1)
    dw 0                      ; Base address (lower 16 bits)
    db 0                      ; Base address (middle 8 bits)
    db 0x89                   ; Access byte (present, DPL=0, executable, accessed)
    db 0x00                   ; Flags (granularity=0, 32-bit)
    db 0                      ; Base address (upper 8 bits)
    dd 0                      ; Base address (upper 32 bits)
    dd 0                      ; Reserved
.end:

section .text
global gdt_flush, load_tss, exception_handler, setup_gdt, initialize_tss

gdt_flush:
    lgdt [gdt_ptr]
    mov ax, 0x10              ; Kernel Data Segment Selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    push 0x08                 ; Kernel Code Segment Selector
    push .flush_done
    retfq
.flush_done:
    ret

load_tss:
    mov ax, 0x28              ; TSS Segment Selector
    ltr ax
    ret

exception_handler:
    cli
    hlt

setup_gdt:
    mov [gdt_ptr], word (gdt.end - gdt - 1)
    mov [gdt_ptr + 2], qword gdt
    call gdt_flush
    call load_tss
    ret

initialize_tss:
    xor rax, rax
    mov ecx, 13               ; Number of qwords to clear
    mov rdi, tss
    rep stosq                 ; Clear TSS structure
    ret
