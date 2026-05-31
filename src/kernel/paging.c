#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "phy_allocator.h"
#include "paging.h"

// high 20 bits for frame number, next 12 bits for flags, format:
// 3 bits for OS use |
// G(PTE/PDE+PS)/X | PS(PDE)/PAT(PTE) | D(dirty, PTE/PDE+PS)/0 | A(accessed) | PCD | PWT
// U/S(user, supervisor) | R/W(read only/write) | P(present) --> important

__attribute__((section(".bootstrap"))) page_table_entry *setup_bootstrap_table(void)
{
    page_table_entry *table = (page_table_entry *)0x91000;

    for (size_t i = 0; i < PAGE_TABLE_SIZE; ++i)
    {
        uint16_t flags = IS_PRESENT | IS_WRITABLE;
        table[i] = (i << 12) | (flags & 0xFFF);
    }
    return table;
}

extern uint32_t __kernel_start, __kernel_end, __kernel_end_lma;
#define PAGE_DIRECTORY_SIZE (1024 * 1024) // one directory entry refers to 4 MB
#define DIRECTORY_INDEX(x) (x >> 22)
#define PAGE_INDEX(x) ((x >> 12) & 0x3FF) // only take 10 bits

bool map_page_to_frame(page_directory_entry *directory, uint32_t fault_address)
{
    void *allocated_frame = allocate_frame();
    if (!allocated_frame)
        return false;

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
    map_page_to_frame(return_page_directory(), fault_address);
    // later add code to terminate the process if mapping failed
}

__attribute__((section(".bootstrap"))) void setup_page_directory_bootstrap(void)
{
    page_directory_entry *directory = (page_directory_entry *)0x90000;
    page_table_entry *table = setup_bootstrap_table();

    uint16_t flags = IS_PRESENT | IS_WRITABLE;
    directory[0] = (uint32_t)table | (flags & 0xFFF); // table is aligned to 4KB so
    // lower 12 bits will anyways be 0

    for (size_t i = 1; i < PAGE_TABLE_SIZE; ++i)
        directory[i] = 0; // set present bit to 0; these tables don't exist
}

page_directory_entry *setup_page_directory_process(void)
{
    page_directory_entry *directory = allocate_frame();
    if (!directory)
        return NULL;

    for (size_t i = 0; i < PAGE_TABLE_SIZE; ++i)
        directory[i] = 0; // set present bit to 0; these tables don't exist

    return directory;
}

size_t residue(size_t num1, size_t num2)
{
    return ((num1) % num2 == 0) ? 0 : 1;
}

#define KERNEL_BASE 0xC0000000 // virtual kernel address
extern char __bootstrap_end;   // physical kernel address
#define KERNEL_TABLE_ADDR 0x92000

void map_kernel_into_process(page_directory_entry *process_directory)
{
    const size_t base_index = KERNEL_BASE / (PAGE_SIZE * PAGE_TABLE_SIZE);
    const size_t end_index = (size_t)(&__kernel_end - 1) / (PAGE_SIZE * PAGE_TABLE_SIZE);
    const uint16_t flags = IS_PRESENT | IS_WRITABLE;

    size_t i = base_index;
    for (; i <= end_index; ++i)
        process_directory[i] = (uint32_t)(KERNEL_TABLE_ADDR + ((i - base_index) * PAGE_TABLE_SIZE * 4)) | (flags & 0xFFF);
}

__attribute__((section(".bootstrap"))) void map_kernel_bootstrap(void)
{
    const size_t base_index = KERNEL_BASE / (PAGE_SIZE * PAGE_TABLE_SIZE);
    const size_t end_index = (size_t)(&__kernel_end - 1) / (PAGE_SIZE * PAGE_TABLE_SIZE);
    const uint16_t flags = IS_PRESENT | IS_WRITABLE;

    page_directory_entry *bootstrap_directory = (page_directory_entry *)0x90000;

    size_t i = base_index;
    for (; i <= end_index; ++i)
        bootstrap_directory[i] = (uint32_t)(KERNEL_TABLE_ADDR + ((i - base_index) * PAGE_TABLE_SIZE * 4)) | (flags & 0xFFF);
}

__attribute__((section(".bootstrap"))) void initiate_kernel_map(void) // creates kernel mapping page tables
{
    uint32_t base_page = KERNEL_BASE / PAGE_SIZE;
    uint32_t end_page = (size_t)(&__kernel_end - 1) / PAGE_SIZE;
    uint32_t base_table = base_page / PAGE_TABLE_SIZE;
    uint32_t end_table = end_page / PAGE_TABLE_SIZE;
    uint16_t flags = IS_PRESENT | IS_WRITABLE;

    for (size_t i = base_table; i <= end_table; ++i)
    {
        page_table_entry *kernel_map_table = (page_table_entry *)(KERNEL_TABLE_ADDR + (4 * PAGE_TABLE_SIZE * (i - base_table))); // size of PTE = 4
        size_t start_index = 0, end_index = 1023;

        if (i == base_table)
            start_index = base_page % PAGE_TABLE_SIZE;
        if (i == end_table)
            end_index = end_page % PAGE_TABLE_SIZE;
        size_t j = 0;

        for (; j < start_index; ++j)
            kernel_map_table[j] = 0;

        for (; j <= end_index; ++j)
        {
            uint32_t offset = ((i - base_table) * PAGE_SIZE * PAGE_TABLE_SIZE) + ((j - start_index) * PAGE_SIZE); // base table always starts at 0 index since KERNEL_BASE is aligned to 4 MB.
            kernel_map_table[j] = (uint32_t)(&__bootstrap_end + offset) | (flags & 0xFFF);
        }
        for (; j < PAGE_TABLE_SIZE; ++j)
            kernel_map_table[j] = 0;
    }
}
