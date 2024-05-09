global tss_flush
section .text
tss_flush:
    mov ax, 0x18
    ltr ax
    ret
