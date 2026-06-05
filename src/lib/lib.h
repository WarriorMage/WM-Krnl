#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

bool print_buffer_to_vga(uint8_t row, uint8_t col, const char *text, uint8_t color, size_t len);

// actually never returns
void exit_process(void);
