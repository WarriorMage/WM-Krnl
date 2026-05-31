#pragma once

#include <stdint.h>
#include <stdbool.h>

#define PAGE_SIZE 4096
#define STACK_END 0xB0000000
#define STACK_BASE 0xC0000000

#define PAGE_TABLE_SIZE 1024
#define IS_PRESENT 0x1
#define IS_WRITABLE 0x2
#define USER_ALLOWED 0x4
typedef uint32_t page_table_entry, page_directory_entry;

typedef struct kernel_table_info
{
    page_table_entry **kernel_tables;
    size_t length;
} kernel_table_info;

extern kernel_table_info kernel_map;

void setup_page_directory_bootstrap(void);
page_directory_entry *setup_page_directory_process(void);
void page_fault_handler(void);

void init_paging(void);

uint32_t load_fault_virtual_address(void);

page_directory_entry *return_page_directory(void);

void initiate_kernel_map(void);

void map_kernel_bootstrap(void);
void map_kernel_into_process(page_directory_entry *process_directory);

bool map_page_to_frame(page_directory_entry *process_directory, uint32_t fault_address);

void switch_to_virtual_stack(void);

size_t residue(size_t num1, size_t num2);
