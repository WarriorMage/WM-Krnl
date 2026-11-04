#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "misc/utilities.h"
#include "drivers/keyboard.h"
#include "memory/paging.h"
#include "process/syscall.h"

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
    volatile char *dest_loc = (volatile char *)((vga_remapped ? VGA_VBASE : VGA_PBASE) + 2 * col + 160 * row);
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
    volatile unsigned short *video = (unsigned short *)VGA_PBASE;

    for (unsigned int i = 0; i < 80 * 25; ++i)
        video[i] = 0x0720;
}

char getchar(void)
{
    uint8_t keyboard_input_var;
    keyboard_read(&keyboard_input_var);
    return scan_code_to_ascii(keyboard_input_var);
}

void kernel_panic(const char *error)
{
    const char message[] = "KERNEL PANIC!";
    size_t i = 0;
    for (; message[i] != '\0'; ++i)
        print_char_to_vga(0, i, message[i], 0x07);
    if (error[0] != '\0')
    {
        print_char_to_vga(0, i, ':', 0x07);
        ++i;
        print_char_to_vga(0, i, ' ', 0x07);
        ++i;
        for (size_t j = 0; error[j] != '\0'; ++j)
        {
            print_char_to_vga(0, i, error[j], 0x07);
            ++i;
        }
    }
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

void memcpy(void *dest, const void *src, size_t bytes)
{
    char *destination = (char *)dest;
    const char *source = (const char *)src;
    for (size_t i = 0; i < bytes; ++i)
        destination[i] = source[i];
}

unsigned int string_length(const char *string)
{
    unsigned int length = 0;
    while (string[length])
        ++length;
    return length;
}

size_t read_line(char *buf, size_t max_len)
{
    if (max_len == 0)
        return 0;
    size_t read_len = 0;
    while (read_len < max_len - 1)
    {
        buf[read_len] = getchar();
        if (buf[read_len] == '\n')
            break;
        ++read_len;
    }
    buf[read_len] = '\0';
    return read_len;
}

void bit_set(uint8_t *base_address, size_t bit_position, alloc_bit bit_value)
{
    uint8_t *target_byte = base_address + (bit_position / 8);
    uint8_t target_bit = (bit_position % 8);
    if (bit_value == USED)
        *target_byte |= (1 << target_bit);
    else
        *target_byte &= ~(1 << target_bit);
}

bool bit_get(uint8_t *base_address, size_t bit_position)
{
    uint8_t *target_byte = base_address + (bit_position / 8);
    uint8_t target_bit = (bit_position % 8);
    return (*target_byte >> target_bit) & 1;
}
