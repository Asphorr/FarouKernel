; boot.asm
bits 16
org 0x7C00

section .text
start:
    ; Setup stack
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00                ; Stack grows downward from 0x7C00
    sti

    ; Print loading message
    mov si, msg_loading
    call print_string

    ; Load kernel from disk
    mov ax, 0x1000                ; ES:BX = 0x1000:0x0000 (0x10000)
    mov es, ax
    xor bx, bx
    
    mov ah, 0x02                  ; Read sectors
    mov al, 10                    ; Number of sectors to read
    mov ch, 0                     ; Cylinder 0
    mov cl, 2                     ; Sector 2 (1-based)
    mov dh, 0                     ; Head 0
    int 0x13
    jc disk_error                 ; Handle disk error

    ; Switch to protected mode
    cli
    lgdt [gdt_descriptor]
    mov eax, cr0
    or al, 1
    mov cr0, eax
    jmp CODE_SEG:protected_mode   ; Far jump to clear pipeline

bits 32
protected_mode:
    ; Set up protected mode segments
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Set up paging for long mode
    call setup_paging

    ; Enable PAE (Physical Address Extension)
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    ; Set EFER.LME (Long Mode Enable)
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    ; Enable paging and enter long mode
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    lgdt [gdt64_descriptor]
    jmp CODE_SEG64:long_mode      ; Far jump to 64-bit segment

setup_paging:
    ; Identity map first 2MB using 2MB pages
    ; PML4 -> PDP -> PD (2MB pages)
    mov edi, pml4                 ; PML4 table at 0x8000
    mov cr3, edi                  ; Set CR3 to PML4
    xor eax, eax
    mov ecx, 0x1000
    rep stosd                     ; Clear page tables

    ; PML4 entry 0 points to PDP
    mov dword [pml4], pdp | 0x03  ; Present + Writeable

    ; PDP entry 0 points to PD
    mov dword [pdp], pd | 0x03    ; Present + Writeable

    ; PD entry 0 is 2MB page
    mov dword [pd], 0x83          ; Present + Writeable + 2MB page
    ret

bits 64
long_mode:
    ; Set up 64-bit segments
    mov ax, DATA_SEG64
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Jump to kernel entry point at 0x10000
    jmp 0x10000

bits 16
print_string:
    lodsb
    test al, al
    jz .done
    mov ah, 0x0E
    int 0x10
    jmp print_string
.done:
    ret

disk_error:
    mov si, msg_disk_error
    call print_string
    cli
    hlt

section .data
msg_loading    db "Loading Kernel...", 0
msg_disk_error db " Disk Error!", 0

section .bss
align 4096
pml4: resb 4096                   ; Page Map Level 4
pdp:  resb 4096                   ; Page Directory Pointer Table
pd:   resb 4096                   ; Page Directory (2MB pages)

section .rodata
; 32-bit GDT
gdt:
.null: equ $ - gdt
    dq 0
.code: equ $ - gdt
    dw 0xFFFF                     ; Limit (bits 0-15)
    dw 0                          ; Base (bits 0-15)
    db 0                          ; Base (bits 16-23)
    db 0x9A                       ; Access byte (PRESENT | RING0 | CODE | EXECUTABLE | READABLE)
    db 0xCF                       ; Flags (4KB granularity) + Limit (bits 16-19)
    db 0                          ; Base (bits 24-31)
.data: equ $ - gdt
    dw 0xFFFF                     ; Limit
    dw 0                          ; Base
    db 0
    db 0x92                       ; Access byte (PRESENT | RING0 | DATA | WRITABLE)
    db 0xCF
    db 0
gdt_descriptor:
    dw $ - gdt - 1
    dd gdt

; 64-bit GDT
gdt64:
.null: equ $ - gdt64
    dq 0
.code: equ $ - gdt64
    dw 0                          ; Limit (ignored)
    dw 0                          ; Base (ignored)
    db 0
    db 0x9A                       ; Access byte
    db 0x20                       ; Flags (LONG_MODE)
    db 0
.data: equ $ - gdt64
    dw 0
    dw 0
    db 0
    db 0x92                       ; Access byte
    db 0
    db 0
gdt64_descriptor:
    dw $ - gdt64 - 1
    dd gdt64

CODE_SEG   equ gdt.code - gdt
DATA_SEG   equ gdt.data - gdt
CODE_SEG64 equ gdt64.code - gdt64
DATA_SEG64 equ gdt64.data - gdt64

times 510 - ($ - $$) db 0
dw 0xAA55
