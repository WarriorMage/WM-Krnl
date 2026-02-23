/* SPDX-License-Identifier: GPL-3.0-or-later */

#include <stdint.h>
#define IDT_SIZE 256

void dummy_handler(void);
void load_idtr(void);
void set_interrupts(void);

typedef struct idt_entry
{
    uint16_t offset_low;
    uint16_t selector;
    uint8_t zero;
    uint8_t type_attr;
    uint16_t offset_high;
} __attribute__((packed)) idt_entry;

typedef struct idtr
{
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) idtr;

idt_entry interrupt_descriptor_table[IDT_SIZE];
idtr idtr_descriptor;

unsigned int string_length(const char *string)
{
    unsigned int length = 0;
    while (string[length])
        length++;
    return length;
}

void print_success(void)
{
    const char *message = "I am great!";
    volatile char *video = (volatile char *)0xB8000;

    for (unsigned int i = 0; i < string_length(message); ++i)
    {
        video[2 * i] = message[i];
        video[2 * i + 1] = 0x07;
    }
}

void clear_screen(void)
{
    volatile unsigned short *video = (unsigned short *)0xB8000;

    for (unsigned int i = 0; i < 80 * 25; ++i)
        video[i] = 0x0720;
}

void fill_idt_entry(uint16_t index, uint32_t handler)
{
    interrupt_descriptor_table[index].offset_low = handler & 0xFFFF;
    interrupt_descriptor_table[index].selector = 0x08;
    interrupt_descriptor_table[index].zero = 0;
    interrupt_descriptor_table[index].type_attr = 0x8E;
    interrupt_descriptor_table[index].offset_high = handler >> 16;
}

void setup_idt(void)
{
    for (unsigned short i = 0; i < IDT_SIZE; ++i)
        fill_idt_entry(i, (uint32_t)dummy_handler);
}

void setup_idtr()
{
    idtr_descriptor.base = (uint32_t)interrupt_descriptor_table;
    idtr_descriptor.limit = sizeof(interrupt_descriptor_table) - 1;

    load_idtr();
}

// ENTRY POINT!
__attribute__((section(".start"))) void kernel_entry(void)
{
    setup_idt();
    setup_idtr();
    clear_screen();
    print_success();

    set_interrupts();

    while (1)
    {
    }
}
