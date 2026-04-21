OUTPUT_FORMAT(elf64-x86-64)
ENTRY(_start)

PHDRS
{
    text    PT_LOAD    FLAGS(5); /* R-X */
    rodata  PT_LOAD    FLAGS(4); /* R-- */
    data    PT_LOAD    FLAGS(6); /* RW- */
}

SECTIONS
{
    /* Limine protocol requires higher-half addresses */
    . = 0xffffffff80000000;

    .text : {
        *(.text .text.*)
    } :text

    . += 0x1000;
    .rodata : {
        *(.rodata .rodata.*)
    } :rodata

    . += 0x1000;
    .data : {
        *(.data .data.*)
    } :data

    .bss : {
        *(.bss .bss.*)
        *(COMMON)
    } :data

    /DISCARD/ : {
        *(.note .note.*)
        *(.comment)
    }
}
