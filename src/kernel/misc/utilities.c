#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "drivers/keyboard.h"
#include "drivers/ata_driver.h"
#include "memory/paging.h"
#include "process/syscall.h"

#define VGA_MEMORY_BASE 0xB8000

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

bool print_char_to_vga(uint8_t row, uint8_t col, uint8_t character, uint8_t color)
{
    if (row >= 25 || col >= 80)
        return false;
    volatile char *dest_loc = (volatile char *)(VGA_VBASE + 2 * col + 160 * row);
    *dest_loc = character;
    *(dest_loc + 1) = color;
    return true;
}

void print_counter(void)
{
    static uint64_t counter = 0;
    char print_buffer[21];
    itoa(counter, print_buffer);
    size_t i = 0;
    for (; print_buffer[i]; ++i)
        print_char_to_vga(1, i, print_buffer[i], 0x07);
    counter++;
}

void clear_screen(void)
{
    volatile unsigned short *video = (unsigned short *)VGA_MEMORY_BASE;

    for (unsigned int i = 0; i < 80 * 25; ++i)
        video[i] = 0x0720;
}

void keyboard_input(void)
{
    uint8_t keyboard_input_var;
    while (1)
    {
        keyboard_read(&keyboard_input_var);
        print_char_to_vga(10, 5, scan_code_to_ascii(keyboard_input_var), 0x07);
    }
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

int32_t __sys_print_buffer_to_vga(syscall_args *arguments)
{
    uint8_t row = (uint8_t)arguments->arg_1;
    uint8_t col = (uint8_t)arguments->arg_2;
    const char *text = (const char *)arguments->arg_3;
    uint8_t color = (uint8_t)arguments->arg_4;
    size_t len = (size_t)arguments->arg_5;

    // uint8_t row, uint8_t col, const char *text, uint8_t color, size_t len
    if (!text)
        return 1;

    // 80 columns per row
    volatile char *dest_loc = (volatile char *)(VGA_VBASE + 2 * col + 2 * 80 * row);
    volatile char *bound = (volatile char *)(VGA_VBASE + (80 * 25 * 2));
    for (size_t i = 0; i < len; ++i)
    {
        if (dest_loc >= bound)
            return 2;
        *dest_loc = text[i];
        *(dest_loc + 1) = color;
        dest_loc += 2;
    }
    return 0;
}

unsigned int string_length(const char *string)
{
    unsigned int length = 0;
    while (string[length])
        length++;
    return length;
}
