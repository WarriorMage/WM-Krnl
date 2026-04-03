#pragma once

#include <stdint.h>

void initialize_kernel_process(void);
void create_process(void (*source_address)(void));
void context_switch(uint32_t current_stack_top);
