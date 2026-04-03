#include <stddef.h>
#include <stdint.h>

#define PAGE_TABLE_SIZE 1024

typedef struct page_table_entry
{
    uint16_t frame_number_high;
    uint16_t frame_low_and_flags;
    // high 4 bytes for frame number LSB, next 12 bytes for flags, format:
    // 3 bits for OS use | G   PS  D(dirty)   A(accessed)   PCD PWT
    // U/S(user, supervisor) R/W(read only/write) P(present) --> important
} page_table_entry;

page_table_entry page_table[PAGE_TABLE_SIZE];
page_table_entry directory[PAGE_TABLE_SIZE];

void fill_page_table(page_table_entry *entry, uint32_t frame_number, uint16_t flags)
{
    entry->frame_number_high = frame_number >> 4;
    entry->frame_low_and_flags = ((frame_number & 0x0F) << 12) | flags;
}

void setup_page_table(void)
{
    for (size_t i = 0; i < PAGE_TABLE_SIZE; ++i)
    {
        uint16_t flags = 0x3;
        fill_page_table(&page_table[i], i, flags);
    }
}

extern uint32_t __kernel_start, __kernel_end;
#define PAGE_DIRECTORY_SIZE (1024 * 1024) // one directory entry refers to 4 MB

void setup_page_directory(void)
{
    uint32_t initial_entry = (uint32_t)&__kernel_start / PAGE_DIRECTORY_SIZE;
    uint32_t final_entry = (uint32_t)&__kernel_end / PAGE_DIRECTORY_SIZE;
    for (uint32_t i = initial_entry; i <= final_entry; ++i)
    {
        setup_page_table();
    }
}
