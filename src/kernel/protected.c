/* SPDX-License-Identifier: GPL-3.0-or-later */

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#define IDT_SIZE 256
#define VGA_MEMORY_BASE 0xB8000

void load_idtr(void);
void clear_interrupts(void);
void set_interrupts(void);
uint8_t inb(uint16_t port);
void outb(uint16_t port, uint8_t value);

#define MASTER_COMMAND 0x20
#define MASTER_DATA 0x21   // Consists of the interrupt mask data for master
#define SLAVE_COMMAND 0xA0 // and slave.
#define SLAVE_DATA 0xA1
#define ICW1 0x11
#define MASTER_BASE 0x20
#define SLAVE_BASE 0x28
#define MODE_8086 0x01
#define EOI 0x20

void remap_pic(void)
{
    uint8_t master_masks = inb(MASTER_DATA);
    uint8_t slave_masks = inb(SLAVE_DATA);

    outb(MASTER_COMMAND, ICW1);
    outb(SLAVE_COMMAND, ICW1);

    outb(MASTER_DATA, MASTER_BASE);
    outb(SLAVE_DATA, SLAVE_BASE);

    outb(MASTER_DATA, 0x04); // Master has 1 slave in IRQ2 (00000'1'00): bit packed
    outb(SLAVE_DATA, 0x02);  // Slave connected to IRQ2 of master

    outb(MASTER_DATA, MODE_8086);
    outb(SLAVE_DATA, MODE_8086);

    outb(MASTER_DATA, master_masks);
    outb(SLAVE_DATA, slave_masks);
}

bool print_char_to_vga(uint8_t row, uint8_t col, uint8_t character, uint8_t color)
{
    if (row >= 25 || col >= 80)
        return false;
    volatile char *dest_loc = (volatile char *)(VGA_MEMORY_BASE + 2 * col + 160 * row);
    *dest_loc = character;
    *(dest_loc + 1) = color;
    return true;
}

void itoa(int32_t value, char *buffer)
{
    size_t i = 0;
    if (value == 0)
    {
        buffer[i++] = '0';
        buffer[i] = '\0';
        return;
    }
    else if (value == INT32_MIN)
    {
        const char *min = "-2147483648";
        while (*min)
            buffer[i++] = *min++;
        buffer[i] = '\0';
        return;
    }
    else if (value < 0)
    {
        buffer[i++] = '-';
        value = -value;
    }
    size_t j = i;
    while (value >= 1)
    {
        buffer[i++] = value % 10 + '0';
        value /= 10;
    }
    buffer[i] = '\0';
    i--;
    while (j < i)
    {
        char temp = buffer[j];
        buffer[j] = buffer[i];
        buffer[i] = temp;
        i--;
        j++;
    }
}

void print_counter(void)
{
    static uint8_t counter = 0;
    char print_buffer[4];
    itoa(counter, print_buffer);
    size_t i = 0;
    for (; print_buffer[i]; ++i)
        print_char_to_vga(1, i, print_buffer[i], 0x07);
    while (i < 4)
    {
        print_char_to_vga(1, i, ' ', 0x07);
        ++i;
    }

    counter++;
}

#define BUFFER_SIZE 128
typedef struct cqueue
{
    uint8_t data[BUFFER_SIZE];
    size_t front, rear, size;
} cqueue;

void keyboard_main(void);
extern cqueue keyboard_buffer;

void interrupt_dispatcher(uint8_t interrupt_number)
{
    switch (interrupt_number)
    {
    case MASTER_BASE:
        print_counter();
        break;
    case MASTER_BASE + 1:
        keyboard_main();
    }
    if (interrupt_number >= MASTER_BASE && interrupt_number < SLAVE_BASE + 8)
    {
        if (interrupt_number >= SLAVE_BASE)
            outb(SLAVE_COMMAND, EOI);
        outb(MASTER_COMMAND, EOI);
    }
}

extern void *isr_table[IDT_SIZE];

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
    const char *message = "Welcome to my kernel!";
    for (unsigned int i = 0; i < string_length(message); ++i)
        print_char_to_vga(0, i, message[i], 0x07);
}

void clear_screen(void)
{
    volatile unsigned short *video = (unsigned short *)VGA_MEMORY_BASE;

    for (unsigned int i = 0; i < 80 * 25; ++i)
        video[i] = 0x0720;
}

#define KERNEL_CODE_SEGMENT 0x08

void fill_idt_entry(uint16_t index, uint32_t handler)
{
    interrupt_descriptor_table[index].offset_low = handler & 0xFFFF;
    interrupt_descriptor_table[index].selector = KERNEL_CODE_SEGMENT;
    interrupt_descriptor_table[index].zero = 0;
    interrupt_descriptor_table[index].type_attr = 0x8E;
    interrupt_descriptor_table[index].offset_high = handler >> 16;
}

void setup_idt(void)
{
    for (unsigned short i = 0; i < IDT_SIZE; ++i)
        fill_idt_entry(i, (uint32_t)isr_table[i]);
}

void setup_idtr()
{
    idtr_descriptor.base = (uint32_t)interrupt_descriptor_table;
    idtr_descriptor.limit = sizeof(interrupt_descriptor_table) - 1;

    load_idtr();
}

bool keyboard_read(uint8_t *scancode)
{
    if (keyboard_buffer.size == 0 || !scancode)
        return false;

    clear_interrupts();
    *scancode = keyboard_buffer.data[(keyboard_buffer.front++) % BUFFER_SIZE];
    keyboard_buffer.size--;
    set_interrupts();

    return true;
}

// ENTRY POINT!
__attribute__((section(".start"))) void kernel_entry(void)
{
    setup_idt();
    setup_idtr();
    clear_screen();
    print_success();
    remap_pic();

    set_interrupts();
    uint8_t read_from_keyboard;

    while (1)
    {
        keyboard_read(&read_from_keyboard);
        print_char_to_vga(2, 0, read_from_keyboard, 0x07);
    }
}
