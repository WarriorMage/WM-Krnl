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
    uint16_t process_id;
    uint32_t page_directory_address;
    uint32_t kernel_stack_top;
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
    process_list[current_process].kernel_stack_top = current_stack_top;
    if (process_list[current_process].state == RUNNING)
        process_list[current_process].state = READY;

    current_process = next_process;
    process_list[current_process].state = RUNNING;
    context_switch_asm(process_list[current_process].page_directory_address, process_list[current_process].kernel_stack_top);
}

uint16_t assign_process_id(void)
{
    // Start processes at 1 since we assume the initial execution state to be process 0
    static uint16_t next_id = 1;
    return next_id++;
}

void switch_cr3(uint32_t new_directory_pa);

bool load_program_to_memory(directory_location process_directory, program_info program)
{
    size_t page_count = program.sector_count / 8 + residue(program.sector_count, 8); // page size = 8 * sector size
    uint32_t current_directory = return_page_directory();                            // returns pa obviously va is just 0xFFFFF000
    uint32_t allocated_frame;
    void *base_page;

    for (size_t i = 0; i < page_count; ++i)
    {
        if (!(allocated_frame = map_page_to_frame(process_directory.directory_va, (i + 1) * PAGE_SIZE))) // leave first page unmapped
            return false;
        void *virtual_page = map_frame_to_page(allocated_frame);
        if (i == 0)
            base_page = virtual_page;
    }
    read_sectors(program.starting_lba, program.sector_count, base_page);
    return true;
}

uint32_t setup_new_stack(uint32_t new_directory_address);

#define PSTACK_BASE 0xC0000000

bool create_process(program_info program)
{
    uint16_t received_pid = assign_process_id();

    directory_location process_directory = setup_page_directory_process();
    if (!process_directory.directory_va)
        return false;

    map_kernel_into_process(process_directory.directory_va);
    initialize_kstack_map(process_directory.directory_va);
    if (!load_program_to_memory(process_directory, program))
        return false;

    if (!map_page_to_frame(process_directory.directory_va, PSTACK_BASE - PAGE_SIZE) || !map_page_to_frame(process_directory.directory_va, KHEAP_BASE))
        return false;

    uint32_t new_stack_top = setup_new_stack((uint32_t)process_directory.directory_pa);
    process_list[received_pid] = (process){READY, received_pid, (uint32_t)process_directory.directory_pa, new_stack_top};
    return true;
}

void exit_process()
{
    process_list[current_process].state = INVALID;
    context_switch(0xC0000000); // process dead, doesn't matter
}
