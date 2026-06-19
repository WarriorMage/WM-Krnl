#pragma once

#include <stdbool.h>

#define FREE 0
#define USED 1

bool bit_get(uint8_t *base_address, size_t bit_position);
void bit_set(uint8_t *base_address, size_t bit_position, bool bit_value);

void read_memory_map_buffer(void);
bool setup_allocator(void);
void *allocate_frame(void);
bool return_frame(uint32_t frame_address);
uint32_t align_up(uint32_t original);
uint32_t align_down(uint32_t original);
