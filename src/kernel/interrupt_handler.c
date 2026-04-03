#include <stdint.h>
#include "io.h"
#include "interrupt_handler.h"
#include "keyboard.h"
#include "sample_processes.h"
#include "process.h"

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

void interrupt_dispatcher(uint8_t interrupt_number, uint32_t esp)
{
    switch (interrupt_number)
    {
    case MASTER_BASE:
        static uint8_t switch_activate = 0;
        print_counter();
        outb(MASTER_COMMAND, EOI);
        switch_activate = (switch_activate + 1) % 100;
        if (switch_activate == 1)
            context_switch(esp);
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

#define IDT_SIZE 256

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

void setup_idtr(void)
{
    idtr_descriptor.base = (uint32_t)interrupt_descriptor_table;
    idtr_descriptor.limit = sizeof(interrupt_descriptor_table) - 1;

    load_idtr();
}
