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
__attribute__((section(".bootstrap"))) void kernel_init(void)
{
    setup_page_directory_bootstrap();
    initiate_kernel_map();
    map_kernel_bootstrap();
    init_paging();
    while (1) // fall through if this returns for some reason
    {
    }
}

__attribute__((section(".start"))) void kernel_cont(void)
{
    clear_bss();
    read_memory_map_buffer();
    if(!setup_allocator())
        kernel_panic();
    if (!map_page_to_frame(PAGE_DIRECTORY_ADDR, 0xEFFFFFFF))
        kernel_panic();
    switch_to_virtual_stack();
}

void kernel_cont2(void)
{
    
    set_pit_frequency(100);
    setup_gdt();
    setup_gdtr();
    setup_idt();
    setup_idtr();
    clear_screen();
    remap_pic();
    print_char_to_vga(10, 5, 'x', 0x07);
    kernel_main();
    while (1)
    {
    }
    
}

void kernel_main(void)
{
    create_process((program_info){100, 1});
    create_process((program_info){101, 1});

    void *heap_ptr = kmalloc(100);
    *(char *)heap_ptr = 'z';
    print_char_to_vga(7, 0, *(char *)heap_ptr, 0x07);
    kfree(heap_ptr);

    // set_interrupts();
    while (1)
    {
    }
}
