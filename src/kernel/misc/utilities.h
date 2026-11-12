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

char getchar(void);
void kernel_panic(const char *error);

int32_t __sys_print_buffer_to_vga(syscall_args *arguments);
unsigned int string_length(const char *string);
int strncmp(const char *s1, const char *s2, size_t len);
void memcpy(void *dest, const void *src, size_t bytes);

typedef enum __alloc_bit{
    FREE,
    USED,
} alloc_bit;
bool bit_get(uint8_t *base_address, size_t bit_position);
void bit_set(uint8_t *base_address, size_t bit_position, alloc_bit bit_value);
