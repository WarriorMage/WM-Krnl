#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define KERNEL_BASE 0xC0000000 // virtual kernel address
#define PAGE_SIZE 4096
#define PAGE_TABLE_SIZE 1024

#define KSTACK_END 0xFFBF0000
#define KSTACK_BASE 0xFFC00000

#define VGA_VBASE 0xFDFF8000

#define IS_PRESENT 0x1
#define IS_WRITABLE 0x2
#define USER_ALLOWED 0x4

#define BOOTSTRAP_DIR_ADDR 0x90000

#define PAGE_MAP_BASE 0xFF000000
#define PAGE_MAP_END  0xFF800000
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
bool initialize_kstack_map(page_directory_entry *process_directory);

void map_kernel_bootstrap(void);
void map_kernel_into_process(page_directory_entry *process_directory);

uint32_t map_page_to_frame(page_directory_entry *process_directory, uint32_t fault_address);
void *map_frame_to_page(uint32_t frame_address);
uint8_t unmap_mftp_page(void *page_address);

void switch_to_virtual_stack(void);

size_t residue(size_t num1, size_t num2);

bool map_region_to_bootstrap(uint32_t base_physical, uint32_t base_virtual, size_t length_in_bytes);
