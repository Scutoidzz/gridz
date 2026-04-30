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

    .limine_reqs : {
        KEEP(*(.limine_reqs_start))
        KEEP(*(.limine_reqs))
        KEEP(*(.limine_reqs_end))
    } :data

    .bss : {
        PROVIDE(bss_start = .);
        *(.bss .bss.*)
        *(COMMON)
        PROVIDE(bss_end = .);
    } :data

    /DISCARD/ : {
        *(.note .note.*)
        *(.comment)
    }
}
