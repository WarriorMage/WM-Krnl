#include <stdint.h>
#include <stdbool.h>
#include "../lib/lib.h"

void process_2(void)
{
    uint8_t i = 79;
    for (int k = 0; k < 500; ++k)
    {
        for (volatile int j = 0; j < 10000000; j++)
            ;
        print_buffer_to_vga(4, (i + 1) % 80, " ", 0x07, 1);
        print_buffer_to_vga(4, i, "B", 0x07, 1);
        i = (i + 79) % 80;
    }
    exit_process();
}