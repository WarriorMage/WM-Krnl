#pragma once

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "process/syscall.h"

#define VGA_PBASE 0xB8000

#define MIN(A, B) ((A) < (B) ? (A) : (B))

void clear_screen(void);
bool print_char_to_vga(uint8_t row, uint8_t col, uint8_t character, uint8_t color);
void print_counter(void);

void keyboard_input(void);
void kernel_panic(const char *error);

int32_t __sys_print_buffer_to_vga(syscall_args *arguments);
unsigned int string_length(const char *string);
void memcpy(void *dest, const void *src, size_t bytes);
