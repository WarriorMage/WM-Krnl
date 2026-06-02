#pragma once
#include <stdbool.h>

void clear_screen(void);
bool print_char_to_vga(uint8_t row, uint8_t col, uint8_t character, uint8_t color);
void print_counter(void);

void keyboard_input(void);
void read_disk_stuff(void);
