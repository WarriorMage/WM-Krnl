/* SPDX-License-Identifier: GPL-3.0-or-later */

SECTIONS
{
/* Assume kernel loaded at 0x10000 and generate all addresses relative to this. */
    __kernel_start = 0x10000;
    . = __kernel_start;
        
/* My .start (kernel_main) should come first at 0x10000 exactly followed by all code. */
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
        __bss_start = .;
        *(.bss)
        /* More uninit vars, mostly for historical purposes. */
        *(COMMON)
        __bss_end = .;
    }

    __kernel_end = .;
}
