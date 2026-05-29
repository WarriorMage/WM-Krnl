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
#include "phy_allocator.h"
#include "vir_allocator.h"
#include "paging.h"

extern char __bss_start;
extern char __bss_end;

void clear_bss(void)
{
    volatile char *bss = &__bss_start;
    while (bss < &__bss_end)
        *bss++ = 0;
}

#define PIT_BASE_DATA 0x40
#define PIT_COMMAND 0x43

void set_pit_frequency(uint32_t frequency)
{
    // The PIT expects a 16 bit divisor, max value is 65536 with a frequency of 18.2 Hz
    // 18.2 Hz is the minimum possible frequency
    uint16_t divisor = 1193182 / frequency;

    outb(PIT_COMMAND, 0x36);
    outb(PIT_BASE_DATA, divisor & 0xFF);
    outb(PIT_BASE_DATA, (divisor >> 8) & 0xFF);
}

void kernel_panic(void)
{
    const char message[] = "KERNEL PANIC!";
    for (size_t i = 0; message[i] != '\0'; ++i)
        print_char_to_vga(0, i, message[i], 0x07);
    while (true)
    {
    }
}

// ENTRY POINT!
__attribute__((section(".start"))) void kernel_init(void)
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
    setup_allocator();
    if(!initiate_kernel_map())
        kernel_panic();
    page_directory_entry *directory_location = setup_page_directory();
    if (directory_location)
        init_paging(directory_location);
    else
        kernel_panic();
    if(!map_page_to_frame(0x900000 - 1))
        kernel_panic();
    switch_to_virtual_stack();
}

void kernel_main(void)
{
    create_process(process_1);
    create_process(process_2);
    create_process(read_disk_stuff);

    void *heap_ptr = kmalloc(100);
    *(char *)heap_ptr = 'z';
    print_char_to_vga(7, 0, *(char *)heap_ptr, 0x07);
    kfree(heap_ptr);

    set_interrupts();
    while (1)
    {
    }
}
