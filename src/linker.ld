/* SPDX-License-Identifier: GPL-3.0-or-later */

SECTIONS
{
/* Assume kernel loaded at 0x10000 and generate all addresses relative to this. */
    __kernel_start = 0x10000;
    . = __kernel_start;
        
/* My .bootstrap (kernel_init) should come first at 0x10000 exactly followed by all code. */
    .bootstrap :
    {
        *(.bootstrap)
    }
    __bootstrap_end = .;

    . = ALIGN(0x1000);    /* have the new section start frame aligned so that */
    __bootstrap_end = .;  /* it can be mapped in the page table.*/

    .text 0xC0000000 : AT(__bootstrap_end)
    {
        *(.start)
        *(.text)
    }

/* Followed by all files' rodata and data. ELF input required to identify these sections. */
    .rodata : AT((LOADADDR(.text) + (ADDR(.rodata) - ADDR(.text))))
    {
        *(.rodata)
    }

    .data : AT((LOADADDR(.text) + (ADDR(.data) - ADDR(.text))))
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
    __kernel_end_lma = LOADADDR(.data) + SIZEOF(.data) + SIZEOF(.bss); /* contains the runtime end including .bss, the kernel page mapping places .bss physically right after .data */
}
