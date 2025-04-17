section .bss
align 16
tss resb 104                    ; TSS structure (x86_64 requires 104 bytes)
gdt_ptr resb 10                 ; GDT pseudo-descriptor (2 bytes limit + 8 bytes base)

section .data
align 16
; Global Descriptor Table (GDT)
gdt:
    dq 0x0000000000000000       ; Null descriptor (required)
    dq 0x00CF9A000000FFFF       ; Kernel CS: DPL=0, 32/64-bit code segment
    dq 0x00CF92000000FFFF       ; Kernel DS: DPL=0, data segment
    dq 0x00CFFA000000FFFF       ; User CS: DPL=3, 32/64-bit code segment
    dq 0x00CFF2000000FFFF       ; User DS: DPL=3, data segment
                                ; TSS descriptor (0x28):
    dw 103                      ; Limit (104-1)
    dw tss & 0xFFFF             ; Base 15:0
    db (tss >> 16) & 0xFF       ; Base 23:16
    db 0x89                     ; P=1, DPL=0, Type=9 (64-bit TSS available)
    db 0x00                     ; Flags (G=0, reserved)
    db (tss >> 24) & 0xFF       ; Base 31:24
    dd (tss >> 32) & 0xFFFFFFFF ; Base 63:32
    dd 0                        ; Reserved

section .text
global gdt_flush, tss_flush, setup_gdt, initialize_tss, setup_tss
extern isr_stub_table

; 优化点1: 使用更高效的TSS初始化
initialize_tss:
    cld                        ; 确保方向标志为正向
    xor eax, eax               ; 清零RAX
    mov rdi, tss               ; TSS起始地址
    mov rcx, 104 / 8           ; 104字节/8 = 13次QWORD存储
    rep stosq                  ; 批量清零TSS结构
    ret

tss_flush:
    mov ax, 0x28               ; TSS选择子 (GDT第6项)
    ltr ax
    ret

setup_tss:
    call initialize_tss
    call tss_flush
    ret

; 优化点2: 优化段寄存器加载顺序
gdt_flush:
    lea rax, [gdt]
    mov word [gdt_ptr], gdt_end - gdt - 1
    mov qword [gdt_ptr + 2], rax

    lgdt [gdt_ptr]             ; 加载GDT

    ; 优化点3: 合并段寄存器加载
    mov ax, 0x10               ; 内核数据段选择子
    mov ds, ax
    mov es, ax
    mov ss, ax
    ; FS/GS通常用于特殊用途，按需单独设置
    xor ax, ax
    mov fs, ax
    mov gs, ax

    jmp 0x08:.flush            ; 远跳转刷新CS
.flush:
    ret

; 优化点4: 消除冗余调用层次
setup_gdt:
    call gdt_flush
    call setup_tss
    ret

gdt_end:
