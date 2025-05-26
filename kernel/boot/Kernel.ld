ENTRY(kernel_main)

SECTIONS
{
    . = 0x100000;

    .text : AT(0x100000)
    {
        *(.text)
    }

    .rodata : AT(0x200000)
    {
        *(.rodata)
    }

    .data : AT(0x300000)
    {
        *(.data)
    }

    .bss : AT(0x400000)
    {
        *(.bss)
    }
}
