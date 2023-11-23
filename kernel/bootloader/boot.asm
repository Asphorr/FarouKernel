[org 0x7c00]

jmp start

%include "bootloader.h"
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
    call load_kernel

    ; Print a message to indicate that we're switching to protected mode
    mov bx, SWITCHING_TO_PM_MSG
    call print_string

    ; Switch to protected mode
    call switch_to_pm

    ; Start the kernel
    call start_protected_mode

; Include the GDT and other constants
%include "gdt.asm"
%include "print_string_pm.asm"

; Global variables
BOOT_DRIVE db 0
LOADING_KERNEL_MSG db "Loading kernel into memory...", 0
SWITCHING_TO_PM_MSG db "Switching to protected mode...", 0

times 510-($-$$) db 0
dw 0xaa55
