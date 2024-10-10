; boot.asm - Improved Version

global start
extern long_mode_start

section .text
    bits 32

; Define constants for readability
PAGE_PRESENT     equ 1
PAGE_WRITABLE    equ 2
PAGE_HUGE        equ 0x80

EFER_MSR         equ 0xC0000080
EFER_LMA         equ (1 << 10) ; Long Mode Active
EFER_LME         equ (1 << 8)  ; Long Mode Enable

; Entry point
start:
    ; Initialize stack
    mov esp, stack_top

    ; Perform system checks
    call check_multiboot
    call check_cpuid
    call check_long_mode

    ; Setup paging structures
    call setup_page_tables
    call enable_paging

    ; Load the 64-bit GDT
    lgdt [gdt64.pointer]

    ; Jump to 64-bit long mode entry point
    jmp gdt64.code:long_mode_start

    ; Unreachable code (optional OK message)
    ; To display after jumping, consider using a different approach
    ; mov dword [0xb8000], 0x4F4B
    ; hlt

;----------------------------------------
; System Check: Multiboot Signature
; Expects: eax contains multiboot magic number
; Returns: void or jumps to error
;----------------------------------------
check_multiboot:
    cmp eax, 0x36d76289
    jne no_multiboot
    ret

no_multiboot:
    mov al, '0'           ; Error code '0'
    jmp error

;----------------------------------------
; System Check: CPUID Support
; Checks if CPUID instruction is supported
; Returns: void or jumps to error
;----------------------------------------
check_cpuid:
    pushfd
    pop eax
    mov ecx, eax

    ; Attempt to toggle ID bit (bit 21) in EFLAGS
    xor eax, (1 << 21)
    push eax
    popfd

    ; Read back EFLAGS
    pushfd
    pop eax

    ; Restore original EFLAGS
    push ecx
    popfd

    ; Compare EFLAGS to see if ID bit changed
    cmp eax, ecx
    je no_cpuid            ; CPUID not supported
    ret

no_cpuid:
    mov al, '1'           ; Error code '1'
    jmp error

;----------------------------------------
; System Check: Long Mode Support
; Uses CPUID to verify long mode capability
; Returns: void or jumps to error
;----------------------------------------
check_long_mode:
    mov eax, 0x80000000    ; Extended CPUID functions
    cpuid
    cmp eax, 0x80000001
    jb no_long_mode        ; CPU too old for long mode

    ; Check long mode flag in CPUID
    mov eax, 0x80000001
    cpuid
    test edx, (1 << 29)     ; EDX bit 29 == LM flag
    jz no_long_mode        ; Long mode not supported

    ret

no_long_mode:
    mov al, '2'           ; Error code '2'
    jmp error

;----------------------------------------
; Setup Page Tables for 64-bit Paging
; Initializes P4, P3, and P2 tables with 2MiB page mappings
;----------------------------------------
setup_page_tables:
    ; Initialize P4 table entry
    mov eax, p3_table
    or eax, PAGE_PRESENT | PAGE_WRITABLE
    mov [p4_table], eax
    mov dword [p4_table + 4], 0    ; Upper 32 bits

    ; Initialize P3 table entry
    mov eax, p2_table
    or eax, PAGE_PRESENT | PAGE_WRITABLE
    mov [p3_table], eax
    mov dword [p3_table + 4], 0    ; Upper 32 bits

    ; Initialize P2 table entries for 2MiB pages
    xor ecx, ecx                    ; Counter = 0

.map_p2_table:
    ; Calculate physical address: ECX * 2MiB
    mov eax, ecx
    shl eax, 21                     ; Multiply by 2^21 to get 2MiB
    or eax, PAGE_PRESENT | PAGE_WRITABLE | PAGE_HUGE

    ; Set P2_table[ECX] = physical address with flags
    mov [p2_table + ecx * 8], eax  ; Lower 32 bits
    mov dword [p2_table + ecx * 8 + 4], 0 ; Upper 32 bits (unused)

    inc ecx
    cmp ecx, 512
    jne .map_p2_table               ; Repeat for all 512 entries

    ret

;----------------------------------------
; Enable Paging and Transition to Long Mode
; Configures control registers and enables paging
;----------------------------------------
enable_paging:
    ; Load P4 table address into CR3
    mov eax, p4_table
    mov cr3, eax

    ; Enable PAE in CR4
    mov eax, cr4
    or eax, (1 << 5)                ; CR4.PAE
    mov cr4, eax

    ; Set EFER.LME to enable Long Mode
    mov ecx, EFER_MSR
    rdmsr
    or eax, EFER_LME
    wrmsr

    ; Enable Paging by setting PG bit in CR0
    mov eax, cr0
    or eax, (1 << 31)               ; CR0.PG
    mov cr0, eax

    ret

;----------------------------------------
; Error Handling
; Displays an error message on the screen and halts
; Input: Error code in AL (ASCII character)
;----------------------------------------
error:
    ; Display "ERR: X" on the screen
    mov dword [0xb8000], 0x52525245      ; "ERR "
    mov byte  [0xb8004], ':'             ; ':'
    mov byte  [0xb8005], ' '             ; ' '
    mov byte  [0xb8006], al              ; Error code
    mov byte  [0xb8007], 0x0F            ; Attribute byte (bright white on black)
    hlt

section .bss
    align 4096
p4_table:
    resb 4096                            ; P4 table (4KiB)
p3_table:
    resb 4096                            ; P3 table (4KiB)
p2_table:
    resb 4096                            ; P2 table (4KiB)
stack_bottom:
    resb 4096 * 4                        ; Reserve 16KiB for stack
stack_top:

section .rodata
    align 16
gdt64:
    dq 0                                 ; Null descriptor
    ; 64-bit Code Segment Descriptor
    dq 0x00AF9A000000FFFF                ; Code segment with appropriate flags
    dq 0x00AF92000000FFFF                ; Data segment with appropriate flags
gdt64.pointer:
    dw gdt64_end - gdt64 - 1            ; Limit
    dq gdt64                             ; Base

gdt64_end:
