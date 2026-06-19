/* SPDX-License-Identifier: GPL-3.0-or-later */

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "misc/io.h"
#include "cpu/gdt_setup.h"
#include "cpu/interrupt_handler.h"
#include "drivers/keyboard.h"
#include "misc/utilities.h"
#include "process/process.h"
#include "memory/phy_allocator.h"
#include "memory/vir_allocator.h"
#include "memory/paging.h"

extern char __bss_start;
extern char __bss_end;

int test_data_value = 73;

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

// ENTRY POINT!
__attribute__((section(".bootstrap_first"))) void kernel_init(void)
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
    if (!setup_allocator())
        kernel_panic();
    if (!initialize_kstack_map((page_directory_entry *)BOOTSTRAP_DIR_ADDR))
        kernel_panic();
    switch_to_virtual_stack();
}

void kernel_main(void); // SHUT UP COMPILER!

void kernel_cont2(void)
{
    set_pit_frequency(100);
    setup_gdt();
    setup_gdtr();
    setup_idt();
    setup_idtr();
    clear_screen();
    remap_pic();
    if (!map_region_to_bootstrap(VGA_PBASE, VGA_VBASE, 32 * 1024))
        kernel_panic();
    kernel_main();
    while (1)
    {
    }
}

#include "drivers/ata_driver.h"

void kernel_main(void)
{

    create_process((program_info){100, 1});
    create_process((program_info){101, 1});

    void *heap_ptr = kmalloc(100);
    *(char *)heap_ptr = 'z';
    print_char_to_vga(7, 0, *(char *)heap_ptr, 0x07);
    kfree(heap_ptr);

    char buf[512];

    write_sectors(200, 1, "Hello World!");
    read_sectors(200, 1, (void *)buf);

    __sys_print_buffer_to_vga(&(syscall_args){10, 5, buf, 0x07, sizeof("Hello World!")});

    set_interrupts();
    while (1)
    {
    }
}
