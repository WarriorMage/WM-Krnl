#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define PAGE_SIZE 4096
#define MEMORY_MAP_BUFFER 0x5000

extern uint32_t __kernel_end;
// Impossible to be 1 always multiple of 4K, so 1 used as sentinel.
static uint32_t next_free_page = 1;

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

#define MAP_ENTRIES_BOUND 32
memory_map_entry memory_map[MAP_ENTRIES_BOUND];
usable_region usable_regions[MAP_ENTRIES_BOUND];

void read_memory_map_buffer(void)
{
    uint16_t entry_count = *(uint16_t *)MEMORY_MAP_BUFFER;
    memory_map_entry *entry = (memory_map_entry *)(MEMORY_MAP_BUFFER + 2);

    uint16_t usable_count = 0;
    for (uint16_t count = 0; count < ((entry_count > MAP_ENTRIES_BOUND) ? MAP_ENTRIES_BOUND : entry_count); ++count)
    {
        memory_map[count] = *entry;
        if(memory_map[count].type == 1){
            usable_regions[usable_count].base = memory_map[count].base;
            usable_regions[usable_count].length = memory_map[count].length;
            usable_count++;
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

typedef struct free_page
{
    struct free_page *prev;
    struct free_page *next;
} free_page;

void setup_allocator(void)
{
    free_page *first_free_page = (free_page *)align_up(&__kernel_end);
    first_free_page->prev = NULL;
    // first_free_page
}

void *get_page()
{
    if (next_free_page == 1)
        next_free_page = align_up(&__kernel_end);
}

bool return_page(void *received_page)
{
    uint32_t page_address = (uint32_t)received_page;
    if (page_address & 0xFFF != 0)
        return false; // Not OS provided aligned page

    uint32_t page_number = page_address / PAGE_SIZE;
    if (page_number < align_up(&__kernel_end) / PAGE_SIZE)
        return false; // Kernel space not managed by allocator

    free_page *freed_page = (free_page *)received_page;
    freed_page->prev = NULL;
    // freed_page->next = head;
}