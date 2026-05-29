#pragma once

#include <stdint.h>
#include <stdbool.h>

#define PAGE_SIZE 4096
#define STACK_END 0x800000
#define STACK_BASE 0x900000
typedef uint32_t page_table_entry, page_directory_entry;

typedef struct kernel_table_info
{
    page_table_entry **kernel_tables;
    size_t length;
} kernel_table_info;

extern kernel_table_info kernel_map;

page_directory_entry *setup_page_directory(void);
void page_fault_handler(void);

void init_paging(page_directory_entry *directory_address);

uint32_t load_fault_virtual_address(void);

page_directory_entry *return_page_directory(void);

bool initiate_kernel_map(void);

void map_kernel_into_process(page_directory_entry *process_directory, const kernel_table_info kernel_map);

bool map_page_to_frame(uint32_t fault_address);

void switch_to_virtual_stack(void);
