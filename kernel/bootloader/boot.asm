[org 0x7c00]

jmp start

%include "print_string.asm"
%include "disk_load.asm"

start:
    ; Set up the data segments
    mov ax, 0x07c0
    mov ds, ax
    mov es, ax

    ; Print a message to indicate that we're loading the kernel
    mov bx, LOADING_KERNEL_MSG
    call print_string

    ; Load the kernel from disk
    mov bx, 0x1000              ; Load the kernel at address 0x1000
    mov dh, 2                   ; Load 2 sectors (we assume the kernel is that big)
    mov dl, [BOOT_DRIVE]        ; Load from the boot drive
    call disk_load

    ; Print a message to indicate that we're switching to protected mode
    mov bx, SWITCHING_TO_PM_MSG
    call print_string

    ; Switch to protected mode
    cli                          ; Disable interrupts
    lgdt [gdt_descriptor]        ; Load the GDT descriptor
    mov eax, cr0                 ; Set the first bit of CR0 to switch to protected mode
    or eax, 0x1
    mov cr0, eax
    jmp CODE_SEG:start_protected_mode   ; Far jump to our 32-bit code

[bits 32]
start_protected_mode:
    ; Set up the data segments
    mov ax, DATA_SEG
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Jump to the kernel
    jmp 0x1000

; Include the GDT and other constants
%include "gdt.asm"
%include "print_string_pm.asm"

; Global variables
BOOT_DRIVE db 0
LOADING_KERNEL_MSG db "Loading kernel into memory...", 0
SWITCHING_TO_PM_MSG db "Switching to protected mode...", 0

times 510-($-$$) db 0
dw 0xaa55
