ENTRY(_start)

SECTIONS
{
    . = 0x100000;

    .text :
    {
        *(.text)
    }

    .rodata :
    {
        *(.rodata)
    }

    .data :
    {
        *(.data)
    }

    .bss :
    {
        *(.bss COMMON)
        *(COMMON)
    }

    .stack (NOLOAD) :
    {
        . = ALIGN(16);
        _stack_start = .;
        . += 16K;
    }
}
