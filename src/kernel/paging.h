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

#define PAGE_MAP_BASE 0xFF000000
#define PAGE_DIRECTORY_ADDR (page_directory_entry *)0xFFFFF000
#define PAGE_TABLE_BASE (page_table_entry *)0xFFC00000
typedef uint32_t page_table_entry, page_directory_entry;

typedef struct directory_location
{
    uint32_t directory_pa;
    page_directory_entry *directory_va;
} directory_location;

void setup_page_directory_bootstrap(void);
directory_location setup_page_directory_process(void);
void page_fault_handler(void);

void init_paging(void);

uint32_t load_fault_virtual_address(void);

uint32_t return_page_directory(void);

void initiate_kernel_map(void);

void map_kernel_bootstrap(void);
void map_kernel_into_process(page_directory_entry *process_directory);

uint32_t map_page_to_frame(page_directory_entry *process_directory, uint32_t fault_address);
void *map_frame_to_page(uint32_t frame_address);

void switch_to_virtual_stack(void);

size_t residue(size_t num1, size_t num2);
