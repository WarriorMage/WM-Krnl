#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "syscall.h"
#include "process.h"
#include "paging.h"
#include "vir_allocator.h"

typedef int32_t (*syscall_t)(syscall_args *);

int32_t __sys_print_buffer_to_vga(syscall_args *arguments);

static const syscall_t syscall_table[5] = {NULL, &__sys_exit_process, NULL, NULL, &__sys_print_buffer_to_vga};

// arg address returns address just under pushad in ISR
void syscall_handler(uint32_t *arg_address)
{
    syscall_args arguments;
    int32_t return_value;
    uint32_t syscall_number = *(arg_address + 7); // eax
    if (syscall_number != 1 && syscall_number != 4)   // only two defined till now
        return_value = -1;
    else
    {
        arguments.arg_1 = *(arg_address + 4); // ebx
        arguments.arg_2 = *(arg_address + 6); // ecx
        arguments.arg_3 = *(arg_address + 5); // edx
        arguments.arg_4 = *(arg_address + 1); // esi
        arguments.arg_5 = *arg_address;       // edi
        arguments.arg_6 = *(arg_address + 2); // ebp

        return_value = syscall_table[syscall_number](&arguments);
    }
    *(arg_address + 7) = return_value; // genius idea
}

int32_t __sys_print_buffer_to_vga(syscall_args *arguments)
{
    uint8_t row = (uint8_t)arguments->arg_1;
    uint8_t col = (uint8_t)arguments->arg_2;
    const char *text = (const char *)arguments->arg_3;
    uint8_t color = (uint8_t)arguments->arg_4;
    size_t len = (size_t)arguments->arg_5;

    // uint8_t row, uint8_t col, const char *text, uint8_t color, size_t len
    if (!text)
        return 1;

    // 80 columns per row
    volatile char *dest_loc = (volatile char *)(VGA_VBASE + 2 * col + 2 * 80 * row);
    volatile char *bound = (volatile char *)(VGA_VBASE + (80 * 25 * 2));
    for (size_t i = 0; i < len; ++i)
    {
        if (dest_loc >= bound)
            return 2;
        *dest_loc = text[i];
        *(dest_loc + 1) = color;
        dest_loc += 2;
    }
    return 0;
}