#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#define VGA_MEMORY_BASE 0xB8000

bool print_to_vga(uint8_t row, uint8_t col, const char *text, uint8_t color, size_t len)
{
    if (!text)
        return false;

    // 80 columns per row
    volatile char *dest_loc = (volatile char *)(VGA_MEMORY_BASE + 2 * col + 2 * 80 * row);
    volatile char *bound = (volatile char *)(VGA_MEMORY_BASE + (80 * 25 * 2));
    for (size_t i = 0; i < len; ++i)
    {
        if (dest_loc >= bound)
            return false;
        *dest_loc = text[i];
        *(dest_loc + 1) = color;
        dest_loc += 2;
    }
    return true;
}