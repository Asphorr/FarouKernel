[org 0x7c00]

; Save the boot drive number
mov [BOOT_DRIVE], dl

; Check if CPUID is supported
mov eax, 0x80000000
cpuid
cmp eax, 0x80000001
jb .no_avx

; Check if AVX is supported
mov eax, 0x80000001
cpuid
test ecx, 1 << 28
jz .no_avx

; Enable AVX
mov eax, cr0
and ax, 0xFFFB      ; Clear coprocessor emulation CR0.EM
or ax, 0x2          ; Set coprocessor monitoring  CR0.MP
mov cr0, eax
mov eax, cr4
or ax, 3 << 9       ; Set CR4.OSFXSR and CR4.OSXMMEXCPT at the same time
mov cr4, eax

; Now you can use AVX instructions

.no_avx:
    ; Print an error message
    mov bx, NO_AVX_MSG
    call print_string
    jmp $

%include "print_string.asm"

BOOT_DRIVE: db 0
NO_AVX_MSG: db 'Error: CPU does not support AVX.', 0

times 510-($-$$) db 0
dw 0xaa55
