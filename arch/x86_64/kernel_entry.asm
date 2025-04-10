section .text
    global _start
    extern kernel_init

_start:
    ; --- Initial Setup ---
    ; Disable interrupts early to prevent unexpected behavior during setup.
    cli

    ; Set up the stack. The stack grows downwards, so stack_top is the initial
    ; value of the stack pointer (rsp).
    mov rsp, stack_top

    ; --- Segment Register Setup ---
    ; In long mode (64-bit), the segment registers (DS, ES, SS) are generally
    ; ignored in favor of the kernel's GDT entries. However, it's good practice
    ; to set them to a known value (data selector) for consistency. FS and GS
    ; are often used for thread-local storage or kernel data structures.
    mov ax, 0x10  ; Example data selector (adjust based on your GDT)
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov gs, ax

    ; Clear the direction flag. This ensures that string operations (like movsb, stosb)
    ; increment the address, which is the standard behavior.
    cld

    ; --- Multiboot Handling (Conditional) ---
    ; Check the multiboot magic number passed by the bootloader (if using multiboot).
    ; If you are not using multiboot, you can remove this section entirely.
    ;
    ; Note: It's better to define multiboot constants for clarity.
    ;       Also, passing the multiboot information structure address in a dedicated
    ;       register (like rdi) is more standard.
    ;
    ; mov eax, magic_number  ; Assume magic_number is defined elsewhere
    ; cmp eax, MULTIBOOT_BOOTLOADER_MAGIC
    ; jne .no_multiboot
    ; mov [multiboot_info], ebx  ; Store the address of the multiboot info structure
    ; jmp .continue_boot

    ; .no_multiboot:
    ;     ; Handle the case where the bootloader is not multiboot compliant.
    ;     ; Consider a more robust error handling mechanism here (e.g., displaying
    ;     ; a specific error code or entering a safe halt state).
    ;     mov esi, no_multiboot_msg
    ;     call print_error  ; Assumes a print_error function exists
    ;     jmp .halt

    ; .continue_boot:

    ; --- Initialization Routines ---
    ; Clear the BSS section (uninitialized data). This ensures that all uninitialized
    ; global variables start with a value of zero.
    call clear_bss

    ; Initialize essential CPU features (e.g., enabling SSE/AVX).
    call cpu_init

    ; --- Kernel Entry Point ---
    ; Call the kernel's main initialization function. This is where your kernel's
    ; primary logic begins.
    call kernel_init

    ; --- Post-Initialization ---
    ; Enable interrupts now that the kernel has been initialized. Be cautious about
    ; enabling interrupts too early, as it can lead to unpredictable behavior.
    sti

    ; --- Halt System ---
    ; If the kernel initialization was successful, the system should ideally enter
    ; a loop or be handled by the kernel's scheduler. This halt loop is a fallback
    ; in case the kernel doesn't explicitly handle system shutdown.
.halt:
    hlt         ; Put the CPU in a low-power state
    jmp .halt   ; Loop indefinitely

clear_bss:
    ; Clears the BSS section by setting all bytes from bss_start to bss_end to zero.
    ; Uses the `rep stosb` instruction for efficiency.
    extern bss_start, bss_end
    mov rdi, bss_start  ; Destination address (start of BSS)
    mov rcx, bss_end    ; End address of BSS
    sub rcx, rdi        ; Calculate the number of bytes to clear
    xor eax, eax        ; Value to write (zero)
    rep stosb           ; Repeat store byte (writes al to [rdi], increments rdi, decrements rcx)
    ret

cpu_init:
    ; Initializes essential CPU features.

    ; Enable SSE (Streaming SIMD Extensions) and FXSAVE/FXRSTOR instructions.
    ; This allows the kernel to use floating-point and SIMD operations.
    mov rax, cr0
    and ax, 0xFFFB  ; Clear the EM (emulation) bit in CR0
    or ax, 0x2      ; Set the MP (monitor coprocessor) bit in CR0
    mov cr0, rax

    mov rax, cr4
    or ax, (1 << 9)  ; Set OSFXSR (Operating System support for FXSAVE/FXRSTOR)
    or ax, (1 << 10) ; Set OSXMMEXCPT (Operating System unmasked SIMD floating-point exceptions)
    mov cr4, rax

    ; Enable AVX (Advanced Vector Extensions) if supported.
    ; This involves setting the XSAVE feature enable bit in XCR0.
    ; You need to check if AVX is supported by the CPU first (using CPUID).
    ; For simplicity, this example assumes AVX support.
    mov ecx, 0
    xor eax, eax
    or eax, (1 << 1)  ; Enable XMM state (required for SSE)
    or eax, (1 << 2)  ; Enable YMM state (for AVX)
    xsetbv          ; Write to XCR0

    ; More CPU initializations can be added here, such as:
    ; - Setting up the Global Descriptor Table (GDT) if not done by the bootloader.
    ; - Initializing paging if not done by the bootloader.
    ; - Enabling other CPU features as needed.

    ret

section .bss
    align 16 ; Align the stack for better performance
stack_bottom:
    resb 16384 ; 16 KiB stack
stack_top:      ; stack_top points to the top of the stack (highest address)

; section .data
; no_multiboot_msg db "Error: No Multiboot-compliant bootloader detected", 0

; section .bss
; multiboot_info: resq 1