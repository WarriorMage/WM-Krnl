#include <stdint.h>
#include <stdbool.h>
#include "../lib/lib.h"

void process_1(void)
{
    uint8_t i = 0;
    for (int k = 0; k < 500; ++k)
    {
        for (volatile int j = 0; j < 10000000; j++)
            ;
        print_buffer_to_vga(3, (i == 0) ? 79 : i - 1, " ", 0x07, 1);
        print_buffer_to_vga(3, i, "A", 0x07, 1);
        i = (i + 1) % 80;
    }
    exit_process();
}