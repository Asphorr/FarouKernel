[org 0x7c00]
[bits 16]

; Basic setup
xor ax, ax
mov ds, ax
mov es, ax
mov ss, ax
mov sp, 0x7C00
mov [boot_drive], dl

; --- Check CPUID availability ---
pushfd
pop eax
mov ebx, eax        ; Save original
xor eax, 1 << 21    ; Flip ID bit
push eax
popfd
pushfd
pop eax
cmp eax, ebx        ; Did it change?
je .no_avx          ; If not, CPUID not supported

; --- Check for AVX support ---
mov eax, 1
cpuid
bt ecx, 28          ; Test AVX bit
jnc .no_avx
bt ecx, 26          ; Test OSXSAVE bit
jnc .no_avx

; --- Enter Protected Mode ---
cli
lgdt [gdt_ptr]
mov eax, cr0
or al, 1
mov cr0, eax
jmp dword 0x08:.pm_enter

[bits 32]
.pm_enter:
    ; Set up segments
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x9000

    ; --- Enable AVX ---
    ; Enable XSAVE
    mov eax, cr4
    or eax, 1 << 18    ; CR4.OSXSAVE
    mov cr4, eax

    ; Set XCR0 to enable x87, SSE, AVX states
    xor ecx, ecx       ; XCR0 register index
    xgetbv
    or eax, 7          ; Enable bits 0-2 
    xsetbv

    ; AVX is now enabled!
    ; Example: Zero YMM0 register
    vpxor ymm0, ymm0, ymm0
    
    jmp $

[bits 16]
.no_avx:
    mov si, msg_error
    call print_string
    cli
    hlt
    jmp $

; String printing routine
print_string:
    lodsb
    test al, al
    jz .done
    mov ah, 0x0E
    int 0x10
    jmp print_string
.done:
    ret

; --- Data ---
boot_drive db 0
msg_error db "Error: AVX not available", 0

; --- GDT (Global Descriptor Table) ---
gdt:
    dq 0                ; Null descriptor
    ; Code segment (0x08)
    dw 0xffff           ; Limit (0-15)
    dw 0                ; Base (0-15)
    db 0                ; Base (16-23)
    db 10011010b        ; Access
    db 11001111b        ; Flags + Limit (16-19)
    db 0                ; Base (24-31)
    ; Data segment (0x10)
    dw 0xffff
    dw 0
    db 0
    db 10010010b
    db 11001111b
    db 0
gdt_end:

gdt_ptr:
    dw gdt_end - gdt - 1  ; Size
    dd gdt                ; Offset

; Boot signature
times 510-($-$$) db 0
dw 0xaa55
