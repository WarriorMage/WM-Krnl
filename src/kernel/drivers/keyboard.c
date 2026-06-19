/* SPDX-License-Identifier: GPL-3.0-or-later */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "misc/io.h"
#include "cpu/interrupt_handler.h"

#define BUFFER_SIZE 128
#define KB_DATA_PORT 0x60
#define KB_STATUS_PORT 0x64

typedef struct cqueue
{
    uint8_t data[BUFFER_SIZE];
    size_t front, rear, size;
} cqueue;

cqueue keyboard_buffer;

const char scan_to_ascii[] = {0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0x08, 0x09, 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', 0x0A, 0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', 0x3B, 0x27, 0x60, 0, 0x5C, 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' '};

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

char scan_code_to_ascii(uint8_t scancode)
{
    if (scancode >= sizeof(scan_to_ascii))
        return 0;
    return scan_to_ascii[scancode];
}
