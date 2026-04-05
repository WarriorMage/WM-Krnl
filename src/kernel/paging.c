#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "allocator.h"

#define PAGE_TABLE_SIZE 1024

typedef uint32_t page_table_entry, page_directory_entry;
// high 20 bits for frame number, next 12 bits for flags, format:
// 3 bits for OS use |
// G(PTE/PDE+PS)/X | PS(PDE)/PAT(PTE) | D(dirty, PTE/PDE+PS)/0 | A(accessed) | PCD | PWT
// U/S(user, supervisor) | R/W(read only/write) | P(present) --> important

page_table_entry *setup_page_table(size_t table_number)
{
    page_table_entry *table = allocate_frame();
    if (!table)
        return NULL;

    for (size_t i = 0; i < PAGE_TABLE_SIZE; ++i)
    {
        uint16_t flags = 0x3;
        table[i] = (table_number * PAGE_TABLE_SIZE + i) << 12 | (flags & 0xFFF);
    }
    return table;
}

uint32_t map_page_to_frame(uint32_t page_number)
{
    return (uint32_t)allocate_frame() >> 12;
}

extern uint32_t __kernel_start, __kernel_end;
#define PAGE_DIRECTORY_SIZE (1024 * 1024) // one directory entry refers to 4 MB

page_directory_entry *setup_page_directory(void)
{
    page_directory_entry *directory = allocate_frame();
    if (!directory)
        return NULL;

    page_table_entry *table = setup_page_table(0);
    if (!table)
        return NULL;

    uint16_t flags = 0x3;
    directory[0] = (uint32_t)table | (flags & 0xFFF); // table is aligned to 4KB so
    // lower 12 bits will anyways be 0

    for (size_t i = 1; i < PAGE_TABLE_SIZE; ++i)
        directory[i] = 0; // set present bit to 0; these tables don't exist

    return directory;
}
