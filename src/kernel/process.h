#pragma once

#include <stdint.h>

typedef struct program_info
{
    uint32_t starting_lba;
    uint32_t sector_count;
} program_info;

extern uint16_t current_process;

void initialize_kernel_process(void);
bool create_process(program_info program);
void context_switch(uint32_t current_stack_top);
