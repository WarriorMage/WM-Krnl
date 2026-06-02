#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "keyboard.h"
#include "read_disk.h"
#include "paging.h"

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

char *vga_memory_base;

bool print_char_to_vga(uint8_t row, uint8_t col, uint8_t character, uint8_t color)
{
    static bool vga_mapped = false;
    if (!vga_mapped)
    {
        if (!(vga_memory_base = map_frame_to_page(0xB8000)))
            return false;
        vga_mapped = true;
    }

    if (row >= 25 || col >= 80)
        return false;
    volatile char *dest_loc = (volatile char *)(vga_memory_base + 2 * col + 160 * row);
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

unsigned int string_length(const char *string)
{
    unsigned int length = 0;
    while (string[length])
        length++;
    return length;
}

void print_success(int process_id)
{
    const char *message;
    if (process_id == 2)
        message = "Hello I am process 2!     ";
    else if (process_id == 1)
        message = "Welcome to this process 1!";
    for (unsigned int i = 0; i < string_length(message); ++i)
        print_char_to_vga(3, i, message[i], 0x07);
}

void process_1(void)
{
    uint8_t i = 0;
    while (1)
    {
        for (volatile int j = 0; j < 10000000; j++)
            ;
        print_char_to_vga(3, (i == 0) ? 79 : i - 1, ' ', 0x07);
        print_char_to_vga(3, i, 'A', 0x07);
        i = (i + 1) % 80;
    }
}

void process_2(void)
{
    uint8_t i = 79;
    while (1)
    {
        for (volatile int j = 0; j < 10000000; j++)
            ;
        print_char_to_vga(4, (i + 1) % 80, ' ', 0x07);
        print_char_to_vga(4, i, 'B', 0x07);
        i = (i + 79) % 80;
    }
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

void read_disk_stuff(void)
{
    char buffer[1024];
    read_sectors(100, 2, buffer);
    for (size_t i = 0; i < sizeof(buffer); ++i)
        print_char_to_vga(6 + (i / 80), i % 80, buffer[i], 0x07);
}