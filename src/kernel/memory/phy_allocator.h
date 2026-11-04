#pragma once

#include <stdint.h>
#include <stdbool.h>

extern bool in_bootstrap;

void read_memory_map_buffer(void);
void setup_allocator(void);
void *allocate_frame(void);
bool return_frame(uint32_t frame_address);
uint32_t align_up(uint32_t original);
uint32_t align_down(uint32_t original);
