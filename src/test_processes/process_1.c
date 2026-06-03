#include <stdint.h>
#include <stdbool.h>

void process_1(void)
{
    uint8_t i = 0;
    bool (*print_to_vga)(uint8_t, uint8_t, uint8_t, uint8_t) =
    (bool (*)(uint8_t, uint8_t, uint8_t, uint8_t))0xC0000A52;

    while (1)
    {
        for (volatile int j = 0; j < 10000000; j++)
            ;
        print_to_vga(3, (i == 0) ? 79 : i - 1, ' ', 0x07);
        print_to_vga(3, i, 'A', 0x07);
        i = (i + 1) % 80;
    }
}