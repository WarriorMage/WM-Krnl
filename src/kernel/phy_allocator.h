#pragma once

void read_memory_map_buffer(void);
void setup_allocator(void);
void *allocate_frame(void);
bool return_page(void *received_page);
