#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "process.h"
#include "paging.h"
#include "phy_allocator.h"

#define MEMORY_MAP_BUFFER 0x5000

extern uint32_t __kernel_start;
extern uint32_t __kernel_end_lma;
extern uint32_t __kernel_end;
extern uint32_t __bss_end;
extern uint32_t __bss_start;

typedef struct memory_map_entry
{
    uint64_t base;
    uint64_t length;
    uint32_t type;
    uint32_t acpi_ext;
} memory_map_entry;

typedef struct usable_region
{
    uint64_t base;
    uint64_t length;
} usable_region;

#define MAP_ENTRIES_BOUND 64
memory_map_entry memory_map[MAP_ENTRIES_BOUND];
uint16_t memory_map_entry_count;
usable_region usable_regions[MAP_ENTRIES_BOUND];
uint16_t usable_region_count;

void read_memory_map_buffer(void)
{
    memory_map_entry_count = *(uint16_t *)MEMORY_MAP_BUFFER;
    memory_map_entry *entry = (memory_map_entry *)(MEMORY_MAP_BUFFER + 2);

    for (uint16_t count = 0; count < ((memory_map_entry_count > MAP_ENTRIES_BOUND) ? MAP_ENTRIES_BOUND : memory_map_entry_count); ++count)
    {
        memory_map[count] = *entry;
        if (memory_map[count].type == 1 && memory_map[count].base >= 0x100000)
        {
            usable_regions[usable_region_count].base = memory_map[count].base;
            usable_regions[usable_region_count].length = memory_map[count].length;
            usable_region_count++;
        }
        entry++;
    }
}

uint32_t align_up(uint32_t original)
{
    // ChatGPT gave the idea, 4095 moves into the next page if offset not 0 else stay
    // in the same page after aligning up since it is entirely free.
    return (original + 4095) & ~0xFFF;
}
uint32_t align_down(uint32_t original)
{
    // Similar. Simple as well.
    return original & ~0xFFF;
}

typedef struct free_page
{
    struct free_page *prev;
    struct free_page *next;
} free_page;

void bit_set(uint8_t *base_address, size_t bit_position, bool bit_value)
{
    uint8_t *target_byte = base_address + (bit_position / 8);
    uint8_t target_bit = (bit_position % 8);
    if (bit_value)
        *target_byte |= (1 << target_bit);
    else
        *target_byte &= ~(1 << target_bit);
}

bool bit_get(uint8_t *base_address, size_t bit_position)
{
    uint8_t *target_byte = base_address + (bit_position / 8);
    uint8_t target_bit = (bit_position % 8);
    return (*target_byte >> target_bit) & 1;
}

#define BITMAP_SIZE (128 * 1024) // max bitmap size in 4 GB systems
uint8_t bitmap[BITMAP_SIZE];
size_t bitmap_size_in_bytes;
uint64_t max_address;

size_t kernel_table_pages;

bool setup_allocator(void)
{
    for (size_t i = 0; i < memory_map_entry_count; ++i)
    {
        uint64_t end_address = memory_map[i].base + memory_map[i].length;
        if (end_address > max_address)
            max_address = end_address;
    }
    if (max_address > UINT32_MAX)
        max_address = UINT32_MAX;

    bitmap_size_in_bytes = align_down(max_address) / PAGE_SIZE / 8;
    for (size_t i = 0; i < bitmap_size_in_bytes; ++i)
    {
        bitmap[i] = 0xFF; // it is ok if i miss out on using upto last 7 pages they will mostly be reserved anyways.
    }

    for (size_t i = 0; i < usable_region_count; ++i)
    {
        size_t j = align_up(usable_regions[i].base);
        for (; j < usable_regions[i].base + usable_regions[i].length; j += PAGE_SIZE)
            bit_set(bitmap, j / PAGE_SIZE, FREE);

        if (j > usable_regions[i].base + usable_regions[i].length) // if last page was partial
        {
            j -= PAGE_SIZE;
            bit_set(bitmap, j / PAGE_SIZE, USED);
        }
    }
    bit_set(bitmap, 0, USED); // leave the first frame unallocated, even if not reserved, use for error catching
    bit_set(bitmap, BOOTSTRAP_DIR_ADDR / PAGE_SIZE, USED); // save bootstrap directory

    kernel_table_pages = ((size_t)(&__kernel_end - 1) / (PAGE_SIZE * PAGE_TABLE_SIZE)) - (KERNEL_BASE / (PAGE_SIZE * PAGE_TABLE_SIZE)) + 1;

    for (size_t i = align_down((uint32_t)&__kernel_start); i < align_up((uint32_t)&__kernel_end_lma) + (kernel_table_pages * PAGE_SIZE); i += PAGE_SIZE)
        bit_set(bitmap, i / PAGE_SIZE, USED);

    return true;
}

void *allocate_frame(void)
{
    for (size_t i = 0; i < bitmap_size_in_bytes * 8; ++i)
    {
        if (bit_get(bitmap, i) == FREE)
        {
            bit_set(bitmap, i, USED);
            return (void *)(i * PAGE_SIZE);
        }
    }

    return NULL; // Protection not enforced, just please don't use this :)
}

bool return_frame(uint32_t frame_address)
{
    if ((frame_address & 0xFFF) != 0)
        return false; // Not OS provided aligned page

    if ((frame_address == BOOTSTRAP_DIR_ADDR && current_process == 0) || frame_address >= max_address || (frame_address >= align_down((uint32_t)&__kernel_start) && frame_address < (uint32_t)&__kernel_end_lma + (kernel_table_pages * PAGE_SIZE)))
        return false; // bootstrap dir freeable after out of bootstrap

    bool returnable = false;
    for (size_t i = 0; i < usable_region_count; ++i)
    {
        if (frame_address >= usable_regions[i].base && frame_address < align_down(usable_regions[i].base + usable_regions[i].length))
        {
            returnable = true;
            break;
        }
    }
    if (!returnable)
        return false;
    size_t bitmap_entry = frame_address / PAGE_SIZE;
    if (bit_get(bitmap, bitmap_entry) == FREE)
        return false; // Double free attempt

    bit_set(bitmap, bitmap_entry, FREE);
    return true;
}
