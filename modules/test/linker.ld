OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)
ENTRY(_start)

PHDRS
{
    text PT_LOAD FLAGS(5);  /* R + X = 5 */
}

SECTIONS
{
    . = 0x400000;

    .text : {
        *(.text .text.*)
        *(.rodata .rodata.*)
    } :text

    .data : {
        *(.data .data.*)
    } :text

    .bss : {
        *(.bss .bss.*)
        *(COMMON)
    } :text

    /DISCARD/ : {
        *(.eh_frame)
        *(.note .note.*)
        *(.comment)
    }
}