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
    init_paging();  // shouldn't return
    kernel_panic("Failed to continue after init paging.");
}

void kernel_main(void); // SHUT UP COMPILER!

__attribute__((section(".start"))) void kernel_cont(void)
{
    clear_bss();
    read_memory_map_buffer();
    setup_allocator();
    set_pit_frequency(100);
    setup_gdt();
    setup_gdtr();
    setup_idt();
    setup_idtr();
    clear_screen();
    remap_pic();
    if (!map_region_to_process((page_directory_entry *)BOOTSTRAP_DIR_ADDR, VGA_PBASE, VGA_VBASE, 32 * 1024))
        kernel_panic("Failed to virtually map VGA");
    vga_remapped = true;
    kernel_main();
    kernel_panic("Should not have returned from kernel_main()?");
}

void kernel_main(void)
{
    create_process((program_info){100, 1});
    create_process((program_info){101, 1});

    // void *heap_ptr = kmalloc(100);
    // *(char *)heap_ptr = 'z';
    // print_char_to_vga(true, 7, 0, *(char *)heap_ptr, 0x07);
    // kfree(heap_ptr);

    // char buf[512];

    // ata_write_sectors(200, 1, "Hello World!");
    // ata_read_sectors(200, 1, (void *)buf);

    // __sys_print_buffer_to_vga(&(syscall_args){10, 5, buf, 0x0F, sizeof("Hello World!"), NULL});

    set_interrupts();
    idle_proc();
}
