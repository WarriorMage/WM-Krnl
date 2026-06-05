#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "phy_allocator.h"
#include "vir_allocator.h"
#include "paging.h"
#include "sample_processes.h"

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
#define DIRECTORY_INDEX(x) ((x) >> 22)
#define PAGE_INDEX(x) (((x) >> 12) & 0x3FF) // only take 10 bits

void test(void);

uint32_t map_page_to_frame(page_directory_entry *directory, uint32_t fault_address)
{
    uint32_t allocated_frame = (uint32_t)allocate_frame();
    if (!allocated_frame)
        return 0;
    page_table_entry *table_va;

    uint16_t flags = IS_PRESENT | IS_WRITABLE;
    if (fault_address < KERNEL_BASE)
        flags |= USER_ALLOWED;

    if (!(directory[DIRECTORY_INDEX(fault_address)] & IS_PRESENT))
    {
        uint32_t new_table_pa = (uint32_t)allocate_frame();
        if (!new_table_pa)
            return false;
        table_va = map_frame_to_page(new_table_pa);
        if (!table_va)
            return false;
        for (size_t i = 0; i < PAGE_TABLE_SIZE; ++i)
            table_va[i] = 0; // Zero the entries or cpu may think unmapped entries as valid

        directory[DIRECTORY_INDEX(fault_address)] = (uint32_t)new_table_pa | flags;
    }
    else
    {
        uint32_t table_pa = (uint32_t)directory[DIRECTORY_INDEX(fault_address)] & ~0xFFF;
        table_va = map_frame_to_page(table_pa);
        if (!table_va)
            return false;
    }

    table_va[PAGE_INDEX(fault_address)] = allocated_frame | flags;
    unmap_mftp_page((void *)table_va);
    test();
    return allocated_frame;
}

void page_fault_handler(void)
{
    // kernel_panic();
}

__attribute__((section(".bootstrap"))) void setup_page_directory_bootstrap(void)
{
    page_directory_entry *directory = (page_directory_entry *)BOOTSTRAP_DIR_ADDR;
    page_table_entry *table = setup_bootstrap_table();

    uint16_t flags = IS_PRESENT | IS_WRITABLE;
    directory[0] = (uint32_t)table | (flags & 0xFFF); // table is aligned to 4KB so
    // lower 12 bits will anyways be 0

    for (size_t i = 1; i < PAGE_TABLE_SIZE - 1; ++i)
        directory[i] = 0; // set present bit to 0; these tables don't exist

    directory[PAGE_TABLE_SIZE - 1] = BOOTSTRAP_DIR_ADDR | (flags & 0xFFF);
    // recursive mapping to virtually access the page tables
}

size_t residue(size_t num1, size_t num2)
{
    return ((num1) % num2 == 0) ? 0 : 1;
}

extern char __bootstrap_end; // physical kernel address

bool initialize_kstack_map(page_directory_entry *process_directory)
{
    // Both end and start addresses are both 4 KB aligned, also it only takes 1 table
    // since the stack grows downward, the 'end' is the 'start' in address space
    const size_t pages = (KSTACK_BASE - KSTACK_END) / PAGE_SIZE;
    for (size_t i = 0; i < pages; ++i)
    {
        if (!map_page_to_frame(process_directory, (KSTACK_END + i * PAGE_SIZE)))
            return false;
    }
    return true;
}

void map_kernel_into_process(page_directory_entry *process_directory)
{
    const size_t base_index = KERNEL_BASE / (PAGE_SIZE * PAGE_TABLE_SIZE);
    const size_t end_index = (PAGE_MAP_END - 1) / (PAGE_SIZE * PAGE_TABLE_SIZE);

    size_t i = base_index;
    for (; i <= end_index; ++i)
        process_directory[i] = (PAGE_DIRECTORY_ADDR)[i];
}

bool map_region_to_bootstrap(uint32_t base_physical, uint32_t base_virtual, size_t length_in_bytes)
{
    const uint32_t end_virtual = base_virtual + length_in_bytes - 1;
    const size_t base_index = base_virtual / (PAGE_SIZE * PAGE_TABLE_SIZE);
    const size_t end_index = end_virtual / (PAGE_SIZE * PAGE_TABLE_SIZE);

    const size_t base_page = (base_virtual / PAGE_SIZE) % PAGE_TABLE_SIZE;
    const size_t end_page = (end_virtual / PAGE_SIZE) % PAGE_TABLE_SIZE;

    page_directory_entry *bootstrap_directory = (page_directory_entry *)BOOTSTRAP_DIR_ADDR;
    uint16_t flags = IS_PRESENT | IS_WRITABLE;

    for (size_t i = base_index; i <= end_index; ++i)
    {
        if (!(bootstrap_directory[i] & IS_PRESENT))
        {
            uint32_t table_pa = (uint32_t)allocate_frame();
            if (!table_pa)
                return false;
            bootstrap_directory[i] = table_pa | (flags & 0xFFF);
        }

        page_table_entry *table_va = (page_table_entry *)((uint32_t)PAGE_TABLE_BASE + (i * PAGE_SIZE));
        for (size_t j = (i == base_index ? base_page : 0); j <= (i == end_index ? end_page : PAGE_TABLE_SIZE); ++j)
        {
            table_va[j] = (base_physical + ((i - base_index) * PAGE_SIZE * PAGE_TABLE_SIZE) + ((j - base_page) * PAGE_SIZE)) | flags;
        }
    }
    return true;
}

__attribute__((section(".bootstrap"))) void map_kernel_bootstrap(void)
{
    const size_t base_index = KERNEL_BASE / (PAGE_SIZE * PAGE_TABLE_SIZE);
    const size_t end_index = (size_t)(&__kernel_end - 1) / (PAGE_SIZE * PAGE_TABLE_SIZE);
    const uint16_t flags = IS_PRESENT | IS_WRITABLE;

    page_directory_entry *bootstrap_directory = (page_directory_entry *)BOOTSTRAP_DIR_ADDR;
    uint32_t kernel_table_pa = ((uint32_t)&__kernel_end_lma + 4095) & ~0xFFF;

    size_t i = base_index;
    for (; i <= end_index; ++i)
        bootstrap_directory[i] = (uint32_t)(kernel_table_pa + ((i - base_index) * PAGE_TABLE_SIZE * 4)) | (flags & 0xFFF);
}

__attribute__((section(".bootstrap"))) void initiate_kernel_map(void) // creates kernel mapping page tables
{
    uint32_t base_page = KERNEL_BASE / PAGE_SIZE;
    uint32_t end_page = (size_t)(&__kernel_end - 1) / PAGE_SIZE;
    uint32_t base_table = base_page / PAGE_TABLE_SIZE;
    uint32_t end_table = end_page / PAGE_TABLE_SIZE;
    uint16_t flags = IS_PRESENT | IS_WRITABLE;

    uint32_t base_table_pa = ((uint32_t)&__kernel_end_lma + 4095) & ~0xFFF;

    for (size_t i = base_table; i <= end_table; ++i)
    {
        page_table_entry *kernel_map_table = (page_table_entry *)(base_table_pa + (4 * PAGE_TABLE_SIZE * (i - base_table))); // size of PTE = 4
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

uint8_t bit_page_map[(PAGE_MAP_END - PAGE_MAP_BASE) / PAGE_SIZE / 8];

void *map_frame_to_page(uint32_t frame_address)
{
    int16_t selected_bit = -1;
    static int16_t prev_selection = -1;
    int16_t i = prev_selection;
    do
    {
        i = (i + 1) % (sizeof(bit_page_map) * 8);
        if (bit_get(bit_page_map, i) == FREE)
        {
            selected_bit = i;
            bit_set(bit_page_map, i, USED);
            break;
        }
    } while (i != prev_selection);

    if (selected_bit == -1)
        return NULL;
    prev_selection = selected_bit;

    uint32_t page_to_return = (PAGE_MAP_BASE + selected_bit * PAGE_SIZE);
    uint16_t flags = IS_PRESENT | IS_WRITABLE;

    size_t directory_index = DIRECTORY_INDEX(page_to_return);
    page_directory_entry *current_directory = (page_directory_entry *)PAGE_DIRECTORY_ADDR;
    page_table_entry *table = (page_table_entry *)((uint32_t)PAGE_TABLE_BASE + (4 * PAGE_TABLE_SIZE * directory_index));
    if (!(current_directory[directory_index] & IS_PRESENT))
    {
        current_directory[directory_index] = (uint32_t)allocate_frame() | (flags & 0xFFF);
        for (size_t i = 0; i < PAGE_TABLE_SIZE; ++i)
            table[i] = 0;
    }

    table[PAGE_INDEX(page_to_return)] = frame_address | (flags & 0xFFF);
    return (void *)page_to_return;
}

uint8_t unmap_mftp_page(void *page_address)
{
    uint32_t page_to_free = align_down((uint32_t)page_address);
    if (page_to_free < PAGE_MAP_BASE || page_to_free > PAGE_MAP_END)
        return 1; // invalid pointer

    uint16_t bit_in_map = (page_to_free - PAGE_MAP_BASE) / PAGE_SIZE;
    if (bit_get(bit_page_map, bit_in_map) == FREE)
        return 2; // double free attempt

    bit_set(bit_page_map, bit_in_map, FREE);

    size_t directory_index = DIRECTORY_INDEX(page_to_free);
    page_directory_entry *current_directory = (page_directory_entry *)PAGE_DIRECTORY_ADDR;
    page_table_entry *table = (page_table_entry *)((uint32_t)PAGE_TABLE_BASE + (4 * PAGE_TABLE_SIZE * directory_index));
    table[PAGE_INDEX(page_to_free)] = 0;

    bool table_free = true;
    for (size_t i = 0; i < PAGE_TABLE_SIZE; ++i)
    {
        if (table[i] != 0)
        {
            table_free = false;
            break;
        }
    }
    if (table_free)
    {
        return_frame(current_directory[directory_index] & (~0xFFF));
        current_directory[directory_index] = 0;
    }
    return 0;
}

directory_location setup_page_directory_process(void)
{
    uint32_t directory = (uint32_t)allocate_frame();
    if (!directory)
        return (directory_location){0, NULL};

    page_directory_entry *directory_va = map_frame_to_page(directory);
    for (size_t i = 0; i < PAGE_TABLE_SIZE - 1; ++i)
        directory_va[i] = 0; // set present bit to 0; these tables don't exist

    // directory guaranteed to be 4 KB aligned
    directory_va[PAGE_TABLE_SIZE - 1] = (uint32_t)directory | ((IS_PRESENT | IS_WRITABLE) & 0xFFF);

    return (directory_location){directory, directory_va};
}

uint32_t get_frame(void *page_address){
    page_table_entry *table_address = PAGE_TABLE_BASE + DIRECTORY_INDEX((uint32_t)page_address) * PAGE_TABLE_SIZE;
    page_table_entry table_entry = table_address[PAGE_INDEX((uint32_t)page_address)];

    if(!(table_entry & IS_PRESENT))
        return 0; // bug catching frame, yay!
    else
        return table_entry & (~0xFFF);
}
