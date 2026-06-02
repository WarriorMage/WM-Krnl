#pragma once

#include <stdbool.h>

void read_memory_map_buffer(void);
bool setup_allocator(void);
void *allocate_frame(void);
bool return_page(void *received_page);
uint32_t align_up(uint32_t original);
uint32_t align_down(uint32_t original);
