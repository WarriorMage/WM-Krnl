#include "lib.h"

typedef enum syscall_number
{
    SYS_EXIT = 1,
    SYS_PRINT = 4
} sys_num;

int32_t sys_gate(uint32_t syscall_number, uint32_t arg_1, uint32_t arg_2, uint32_t arg_3, uint32_t arg_4, uint32_t arg_5, uint32_t arg_6);

void exit_process(void)
{
    sys_gate(SYS_EXIT, 0, 0, 0, 0, 0, 0);
}

bool print_buffer_to_vga(uint8_t row, uint8_t col, const char *text, uint8_t color, size_t len)
{
    return (bool)sys_gate(SYS_PRINT, row, col, (uint32_t)text, color, len, 0);
}