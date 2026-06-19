#pragma once

#include <stdint.h>
#include <stdbool.h>

void keyboard_main(void);
bool keyboard_read(uint8_t *scancode);
char scan_code_to_ascii(uint8_t scancode);
