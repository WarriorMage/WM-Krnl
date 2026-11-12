#pragma once

#include <stdint.h>
#include <stdbool.h>

extern uint8_t *dbitmap_cache;
uint32_t bitmap_bit_to_block(uint32_t bit);
uint32_t dbitmap_alloc(void);
bool dbitmap_free(uint32_t block_num);
