#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "process.h"
#include "paging.h"
#include "read_disk.h"
#include "vir_allocator.h"

typedef enum process_state
{
    INVALID, // Keep this as 0 since all entries will have to be INVALID at start
    // Which is what BSS does automatically, change this and won't work
    READY,
    RUNNING,
    BLOCKED, // For later
} process_state;

// For scheduling
typedef struct process
{
    process_state state;
    uint8_t process_id;
    uint32_t page_directory_address;
    uint32_t stack_top;
} process;

// Process storage
#define MAX_PROCESSES 256
process process_list[MAX_PROCESSES];

// Not supposed to return
void context_switch_asm(uint32_t new_directory_address, uint32_t new_stack_top);

uint8_t current_process = 0;

// fix infinite loop when no other process is ready

void context_switch(uint32_t current_stack_top)
{
    uint8_t next_process = (current_process + 1) % MAX_PROCESSES;
    while (process_list[next_process].state != READY)
        next_process = (next_process + 1) % MAX_PROCESSES;
    process_list[current_process].stack_top = current_stack_top;
    if (process_list[current_process].state == RUNNING)
        process_list[current_process].state = READY;

    current_process = next_process;
    process_list[current_process].state = RUNNING;
    context_switch_asm(process_list[current_process].page_directory_address, process_list[current_process].stack_top);
}

uint8_t assign_process_id(void)
{
    // Start processes at 1 since we assume the initial execution state to be process 0
    static uint8_t next_id = 1;
    return next_id++;
}

bool load_program_to_memory(page_directory_entry *process_directory, program_info program)
{
    size_t page_count = program.sector_count / 8 + residue(program.sector_count, 8); // page size = 8 * sector size
    for (size_t i = 0; i < page_count; ++i)
    {
        if (!map_page_to_frame(process_directory, (i + 1) * PAGE_SIZE)) // leave first page unmapped
            return false;
    }
    read_sectors(program.starting_lba, program.sector_count, (void *)(1 * PAGE_SIZE));
    return true;
}

void setup_new_stack(uint32_t new_directory_address);

bool create_process(program_info program)
{
    uint8_t received_pid = assign_process_id();

    page_directory_entry *process_directory = setup_page_directory_process();
    if (!process_directory)
        return false;

    map_kernel_into_process(process_directory);
    load_program_to_memory(process_directory, program);

    if (!map_page_to_frame(process_directory, STACK_BASE - PAGE_SIZE) || !map_page_to_frame(process_directory, HEAP_BASE))
        return false;

    setup_new_stack((uint32_t)process_directory);
    process_list[received_pid] = (process){READY, received_pid, (uint32_t)process_directory, 0xC0000000};
    return true;
}

void exit_process()
{
    process_list[current_process].state = INVALID;
    context_switch(0x90000000); // process dead, doesn't matter
}
