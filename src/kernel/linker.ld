OUTPUT_FORMAT(binary)
ENTRY (KStart)

INPUT
(
    ../../build/stage2.o
    ../../build/main.o
    ../../build/IDT.o
)

OUTPUT
(
    ../../build/kernel.bin
)

SECTIONS
{
    . = 0x8000;

    .text : ALIGN(0x1000)
    {
        *(.text)
    }

    .idt BLOCK(0x1000) : ALIGN(0x1000)
    {
        _idt = .;
        . = . + 0x1000;
    }

    .data : ALIGN(0x1000)
    {
        *(.data)
    }

    .rodata : ALIGN(0x1000)
    {
        *(.rodata)
    }

    .bss : ALIGN(0x1000)
    {
        *(COMMON)
        *(.bss)
    }
}