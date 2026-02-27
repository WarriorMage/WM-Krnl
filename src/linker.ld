/* SPDX-License-Identifier: GPL-3.0-or-later */

OUTPUT_FORMAT(binary)

SECTIONS
{
/* Assume kernel loaded at 0x10000 and generate all addresses relative to this. */
    . = 0x10000;    

/* My .start (kernel_entry) should come first at 0x10000 exactly followed by all code. */
    .text :
    {
        *(.start)
        *(.text)
    }

/* Followed by all files' rodata and data. ELF input required to identify these sections. */
    .rodata :
    {
        *(.rodata)
    }

    .data :
    {
        *(.data)
    }

/* bss gets no space in final binary, it just affects memory offsets, so that it gets space at runtime. */
    .bss :
    {
        *(.bss)
        /* More uninit vars, mostly for historical purposes. */
        *(COMMON)
    }
}
