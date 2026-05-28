#pragma once

void clear_screen(void);
bool print_char_to_vga(uint8_t row, uint8_t col, uint8_t character, uint8_t color);
void print_counter(void);

void process_1(void);
void process_2(void);
void keyboard_input(void);
