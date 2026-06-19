#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "process/syscall.h"
#include "process/process.h"
#include "memory/paging.h"
#include "memory/vir_allocator.h"
#include "misc/utilities.h"

typedef int32_t (*syscall_t)(syscall_args *);

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