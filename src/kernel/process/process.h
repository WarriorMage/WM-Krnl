#pragma once

#include <stdint.h>
#include "process/syscall.h"

typedef struct program_info
{
    uint32_t starting_lba;
    uint32_t sector_count;
} program_info;

extern uint16_t current_process;

#define PSTACK_BASE 0xC0000000
#define PSTACK_END 0xBFF00000 // 1 MB user stack

void initialize_kernel_process(void);
bool create_process(program_info program);
void context_switch(uint32_t current_stack_top);
int32_t __sys_exit_process(syscall_args *arguments);
