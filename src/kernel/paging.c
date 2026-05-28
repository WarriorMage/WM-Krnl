#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "phy_allocator.h"
#include "paging.h"

#define PAGE_TABLE_SIZE 1024
#define IS_PRESENT 0x1
#define IS_WRITABLE 0x2
#define USER_ALLOWED 0x4

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
        uint16_t flags = IS_PRESENT | IS_WRITABLE;
        table[i] = (table_number * PAGE_TABLE_SIZE + i) << 12 | (flags & 0xFFF);
    }
    return table;
}

extern uint32_t __kernel_start, __kernel_end;
#define PAGE_DIRECTORY_SIZE (1024 * 1024) // one directory entry refers to 4 MB
#define DIRECTORY_INDEX(x) (x >> 22)
#define PAGE_INDEX(x) ((x >> 12) & 0x3FF) // only take 10 bits

bool map_page_to_frame(uint32_t fault_address)
{
    void *allocated_frame = allocate_frame();
    if (!allocated_frame)
        return false;

    page_directory_entry *directory = return_page_directory();

    if (!(directory[DIRECTORY_INDEX(fault_address)] & IS_PRESENT))
    {
        page_table_entry *new_table = (page_table_entry *)allocate_frame();
        if (!new_table)
            return false;
        for (size_t i = 0; i < PAGE_TABLE_SIZE; ++i)
            new_table[i] = 0; // Zero the entries or cpu may think unmapped entries as valid

        directory[DIRECTORY_INDEX(fault_address)] = (uint32_t)new_table | IS_PRESENT | IS_WRITABLE;
    }

    page_table_entry *table = (page_table_entry *)((uint32_t)directory[DIRECTORY_INDEX(fault_address)] & ~0xFFF);

    table[PAGE_INDEX(fault_address)] = (uint32_t)allocated_frame | IS_PRESENT | IS_WRITABLE;
    return true;
}

void page_fault_handler(void)
{
    uint32_t fault_address = load_fault_virtual_address() & ~0xFFF;
    map_page_to_frame(fault_address);
    // later add code to terminate the process if mapping failed
}

page_directory_entry *setup_page_directory(void)
{
    page_directory_entry *directory = allocate_frame();
    if (!directory)
        return NULL;

    page_table_entry *table = setup_page_table(0);
    if (!table)
        return NULL;

    uint16_t flags = IS_PRESENT | IS_WRITABLE;
    directory[0] = (uint32_t)table | (flags & 0xFFF); // table is aligned to 4KB so
    // lower 12 bits will anyways be 0

    for (size_t i = 1; i < PAGE_TABLE_SIZE; ++i)
        directory[i] = 0; // set present bit to 0; these tables don't exist

    return directory;
}

size_t residue(size_t num1, size_t num2)
{
    return ((num1) % num2 == 0) ? 0 : 1;
}

#define KERNEL_BASE 0xC0000000

typedef struct kernel_table_info
{
    page_table_entry **kernel_tables;
    size_t length;
} kernel_table_info;

void map_kernel_into_process(page_directory_entry *process_directory, kernel_table_info kernel_map)
{
    static const size_t base_index = (KERNEL_BASE / (PAGE_SIZE * PAGE_TABLE_SIZE));
    static const uint16_t flags = IS_PRESENT | IS_WRITABLE;

    size_t i = 0;
    for (; i < kernel_map.length; ++i)
        process_directory[base_index + i] = (uint32_t)kernel_map.kernel_tables[i] | (flags & 0xFFF);
    for (; i < PAGE_DIRECTORY_SIZE - base_index; ++i)
        process_directory[base_index + i] = 0;
}

kernel_table_info initiate_kernel_map(void)
{
    page_table_entry **kernel_tables = allocate_frame();
    size_t table_size = 0;

    size_t kernel_page_count = ((&__kernel_end - &__kernel_start) / PAGE_SIZE) + residue(&__kernel_end - &__kernel_start, PAGE_SIZE);
    size_t kernel_page_table_count = (kernel_page_count / PAGE_TABLE_SIZE) + residue(kernel_page_count, PAGE_TABLE_SIZE);
    uint16_t flags = IS_PRESENT | IS_WRITABLE;

    for (size_t i = 0; i < kernel_page_table_count; ++i)
    {
        page_table_entry *kernel_map = allocate_frame();
        ++table_size;
        kernel_tables[i] = kernel_map;
        if (!kernel_map)
            return (kernel_table_info){NULL, 0};

        size_t table_entries = (i == kernel_page_table_count - 1) ? (kernel_page_count - (kernel_page_table_count - 1) * PAGE_TABLE_SIZE) : PAGE_TABLE_SIZE;
        size_t j = 0;

        for (; j < table_entries; ++j)
        {
            uint32_t offset = (i * PAGE_SIZE * PAGE_TABLE_SIZE) + (j * PAGE_SIZE);
            kernel_map[j] = (uint32_t)(&__kernel_start + offset) | (flags & 0xFFF);
        }
        for (; j < PAGE_TABLE_SIZE; ++j)
            kernel_map[j] = 0;
    }

    return (kernel_table_info){kernel_tables, table_size};
}
