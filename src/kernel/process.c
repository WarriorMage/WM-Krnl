#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef enum process_state
{
    INVALID, // Keep this as 0 since all entries will have to be INVALID at start
    // Which is what BSS does automatically, change this and won't work
    READY,
    RUNNING,
    BLOCK, // For later
} process_state;

// For scheduling
typedef struct process
{
    process_state state;
    uint8_t process_id;
    uint32_t kernel_stack_top;
} process;

// Process storage
#define MAX_PROCESSES 256
process process_list[MAX_PROCESSES];

// Not supposed to return
void context_switch_asm(uint32_t new_stack_top);

uint8_t current_process = 0;

void context_switch(uint32_t current_stack_top)
{
    uint8_t next_process = current_process + 1;
    while (process_list[next_process].state != READY)
        next_process = (next_process + 1) % 256;
    process_list[current_process].kernel_stack_top = current_stack_top;
    if (process_list[current_process].state == RUNNING)
        process_list[current_process].state = READY;

    current_process = next_process;
    process_list[current_process].state = RUNNING;
    context_switch_asm(process_list[current_process].kernel_stack_top);
}

void initialize_kernel_process(void)
{
    process_list[0].process_id = 0;
    process_list[0].state = INVALID;
    process_list[0].kernel_stack_top = 0x90000;
}

uint8_t assign_process_id(void)
{
    // Start processes at 1 since we assume the initial execution state to be process 0
    static uint8_t next_id = 1;
    return next_id++;
}

#define PROCESS_STACK_SIZE 8192 // 8 KB stack per process
uint32_t assign_stack(uint8_t process_id)
{
    return 0x90000 - (process_id * PROCESS_STACK_SIZE);
}

uint32_t setup_new_stack(uint32_t new_stack_top, void (*source_address)(void));

// Applies on current exiting process
void destroy_stack();

void create_process(void (*source_address)(void))
{
    uint8_t received_pid = assign_process_id();
    uint32_t new_stack_top = setup_new_stack(assign_stack(received_pid), source_address);
    process_list[received_pid] = (process){READY, received_pid, new_stack_top};
}

void exit_process()
{
    process_list[current_process].state = INVALID;
    context_switch(assign_stack(current_process));
}
