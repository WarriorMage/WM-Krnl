#pragma once

#include <stdint.h>

typedef struct syscall_args
{
    uint32_t arg_1; // Argument 1 - 6 till bottom
    uint32_t arg_2; // Don't use ebp in syscall user wrapper till leave where the old
    uint32_t arg_3; // one is restored
    uint32_t arg_4;
    uint32_t arg_5;
    uint32_t arg_6;
} syscall_args;

void syscall_handler(uint32_t *arg_address);
