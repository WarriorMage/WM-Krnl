/* SPDX-License-Identifier: GPL-3.0-or-later */

#include <stdint.h>
#include <stddef.h>

#define BUFFER_SIZE 128
#define KB_DATA_PORT 0x60
#define KB_STATUS_PORT 0x64

typedef struct cqueue
{
    uint8_t data[BUFFER_SIZE];
    size_t front, rear, size;
} cqueue;

cqueue keyboard_buffer;

uint8_t inb(uint16_t port);
void outb(uint16_t port, uint8_t value);

void keyboard_main(void)
{
    while (inb(KB_STATUS_PORT) & 1)
    {
        if (keyboard_buffer.size == BUFFER_SIZE)
            return;
        uint8_t kb_value = inb(KB_DATA_PORT);
        keyboard_buffer.data[(keyboard_buffer.rear++) % BUFFER_SIZE] = kb_value;
        keyboard_buffer.size++;
    }
}