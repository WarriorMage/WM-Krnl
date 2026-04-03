/* SPDX-License-Identifier: GPL-3.0-or-later */

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "io.h"
#include "gdt_setup.h"
#include "interrupt_handler.h"
#include "keyboard.h"
#include "sample_processes.h"
#include "process.h"
#include "allocator.c"

extern char __bss_start;
extern char __bss_end;

void clear_bss(void)
{
    char *bss = &__bss_start;
    while (bss < &__bss_end)
        *bss++ = 0;
}

void set_pit_frequency(uint32_t frequency)
{
    // The PIT expects a 16 bit divisor, max value is 65536 with a frequency of 18.2 Hz
    // 18.2 Hz is the minimum possible frequency
    uint16_t divisor = 1193182 / frequency;

    outb(0x43, 0x36);
    outb(0x40, divisor & 0xFF);
    outb(0x40, (divisor >> 8) & 0xFF);
}

// ENTRY POINT!
__attribute__((section(".start"))) void kernel_main(void)
{
    initialize_kernel_process();
    set_pit_frequency(100);
    clear_bss();
    setup_gdt();
    setup_gdtr();
    setup_idt();
    setup_idtr();
    clear_screen();
    remap_pic();
    read_memory_map_buffer();

    create_process(process_1);
    create_process(process_2);

    set_interrupts();
    uint8_t read_from_keyboard;

    while (1)
    {
        keyboard_read(&read_from_keyboard);
        print_char_to_vga(2, 0, scan_code_to_ascii(read_from_keyboard), 0x07);
    }
}
