#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "syscall.h"
#include "process.h"
#include "paging.h"
#include "read_disk.h"
#include "vir_allocator.h"
#include "interrupt_handler.h"

typedef enum process_state
{
    INVALID, // Keep this as 0 since all entries will have to be INVALID at start
    // Which is what BSS does automatically, change this and won't work
    READY,
    RUNNING,
    BLOCKED, // For later
} process_state;

typedef struct cleanup
{
    bool pending;
    uint16_t process_id;
} cleanup;

static cleanup to_clean_process;

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
static process process_list[MAX_PROCESSES];

// Not supposed to return
void context_switch_asm(uint32_t new_directory_address, uint32_t new_stack_top);

uint16_t current_process = 0;

// fix infinite loop when no other process is ready

void context_switch(uint32_t current_stack_top)
{
    if (process_list[current_process].state == RUNNING)
        process_list[current_process].state = READY;
    uint8_t next_process = (current_process + 1) % MAX_PROCESSES;
    while (process_list[next_process].state != READY)
        next_process = (next_process + 1) % MAX_PROCESSES;
    process_list[current_process].kernel_stack_top = current_stack_top;

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

bool load_program_to_memory(directory_location process_directory, program_info program)
{
    size_t page_count = program.sector_count / 8 + residue(program.sector_count, 8); // page size = 8 * sector size
    uint32_t allocated_frame;
    
    for (size_t i = 0; i < page_count; ++i)
    {
        if (!(allocated_frame = map_page_to_frame(process_directory.directory_va, (i + 1) * PAGE_SIZE))) // leave first page unmapped
            return false;

        void *virtual_page = map_frame_to_page(allocated_frame);
        read_sectors(program.starting_lba + (i * 8), (i == page_count - 1) ? program.sector_count - (i * 8) : 8, virtual_page);
        unmap_mftp_page(virtual_page);
    }
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

    if (!map_page_to_frame(process_directory.directory_va, PSTACK_BASE - PAGE_SIZE))
        return false;

    uint32_t new_stack_top = setup_new_stack((uint32_t)process_directory.directory_pa);
    process_list[received_pid] = (process){READY, received_pid, (uint32_t)process_directory.directory_pa, new_stack_top};
    return true;
}

int32_t __sys_exit_process(syscall_args *arguments)
{
    (void)arguments; // to shut up the compiler
    for (size_t i = 0; i < (KERNEL_BASE / (PAGE_SIZE * PAGE_TABLE_SIZE)); ++i)
    {
        page_directory_entry d_entry = (PAGE_DIRECTORY_ADDR)[i];
        if (d_entry & IS_PRESENT)
        {
            page_table_entry *t_address = PAGE_TABLE_BASE + i * PAGE_TABLE_SIZE;
            for (size_t j = 0; j < PAGE_TABLE_SIZE; ++j)
            {
                page_table_entry t_entry = t_address[j];
                if (t_entry & IS_PRESENT)
                {
                    return_frame(t_entry & (~0xFFF));
                    t_address[j] = 0;
                }
            }
            return_frame(d_entry & (~0xFFF));
            (PAGE_DIRECTORY_ADDR)[i] = 0;
        }
    }

    clear_interrupts();
    process_list[current_process].state = INVALID; // we get messed up if timer
    to_clean_process.pending = true;               // ticks in middle of these operations
    to_clean_process.process_id = current_process;
    set_interrupts();

    while (1) // wait for the timer and escape
    {
    }

    return 0; // shut up the compiler it actually never returns
}

void dead_process_cleanup(void)
{
    if (!to_clean_process.pending)
        return;

    // if timer ticks middle way, we can possibly double free frames, but we don't have
    // to worry here since we are inside isr_32 where IF = 0 anyways

    uint32_t page_directory_pa = process_list[to_clean_process.process_id].page_directory_address;
    page_directory_entry *page_directory_va = map_frame_to_page(page_directory_pa);

    // last recursive entry is just the directory, cleaned at last
    for (size_t i = (PAGE_MAP_END / (PAGE_SIZE * PAGE_TABLE_SIZE)); i < 1023; ++i)
    {
        page_directory_entry d_entry = page_directory_va[i];
        if (d_entry & IS_PRESENT)
        {
            uint32_t page_table_pa = d_entry & (~0xFFF);
            page_table_entry *page_table_va = map_frame_to_page(page_table_pa);
            for (size_t j = 0; j < PAGE_TABLE_SIZE; ++j)
            {
                page_table_entry t_entry = page_table_va[j];
                if (t_entry & IS_PRESENT)
                {
                    return_frame(t_entry & (~0xFFF));
                    // don't need to zero entries, that directory and tables are dead anyways
                }
            }
            return_frame(d_entry & (~0xFFF));
        }
    }

    return_frame(page_directory_pa);
    to_clean_process.pending = false;
}
