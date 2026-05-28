#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "paging.h"

#define MEMORY_MAP_BUFFER 0x5000

extern uint32_t __kernel_start;
extern uint32_t __kernel_end;
// // Impossible to be 1 always multiple of 4K, so 1 used as sentinel.
// static uint32_t next_free_page = 1;

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

// void check_page_status(const void *base_address) ; idk what i wrote this for, maybe it will help later.
// {
//     base_address = (void *)((uint32_t)base_address & ~0xFFF);
// }

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

#define FREE 0
#define USED 1
uint8_t *bitmap = (uint8_t *)0x100000;
size_t bitmap_size_in_bytes;
uint64_t max_address;

void setup_allocator(void)
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

    for (size_t i = 0; i < 0x100000 / PAGE_SIZE; ++i)
    {
        bit_set(bitmap, i, USED);
    }

    for (size_t i = align_down((uint32_t)&__kernel_start); i < (uint32_t)&__kernel_end; i += PAGE_SIZE)
        bit_set(bitmap, i / PAGE_SIZE, USED);

    size_t bitmap_base_page = (uint32_t)bitmap / PAGE_SIZE;
    for (size_t i = 0; i < (bitmap_size_in_bytes / PAGE_SIZE) + ((bitmap_size_in_bytes & 0xFFF) ? 1 : 0); ++i)
        bit_set(bitmap, bitmap_base_page + i, USED);
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

bool return_frame(void *received_page)
{
    uint32_t page_address = (uint32_t)received_page;
    if ((page_address & 0xFFF) != 0)
        return false; // Not OS provided aligned page

    if (page_address < 0x100000 || page_address >= max_address || (page_address >= align_down((uint32_t)&__kernel_start) && page_address < (uint32_t)&__kernel_end) || (page_address >= align_down((uint32_t)bitmap) && page_address < (uint32_t)bitmap + bitmap_size_in_bytes))
        return false;

    bool returnable = false;
    for (size_t i = 0; i < usable_region_count; ++i)
    {
        if (page_address >= usable_regions[i].base && page_address < align_down(usable_regions[i].base + usable_regions[i].length))
        {
            returnable = true;
            break;
        }
    }
    if (!returnable)
        return false;
    size_t bitmap_entry = page_address / PAGE_SIZE;
    if (bit_get(bitmap, bitmap_entry) == FREE)
        return false; // Double free attempt

    bit_set(bitmap, bitmap_entry, FREE);
    return true;
}
