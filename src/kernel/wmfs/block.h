#pragma once

#include <stdint.h>
#include <stdbool.h>
#define BLOCK_SIZE 4096

typedef struct __disk_info_t
{
    uint64_t sector_count;
} disk_info_t;

bool read_blocks(uint32_t block_number, uint8_t block_count, void *buf);
bool write_blocks(uint32_t block_number, uint8_t block_count, const void *buf);
bool get_disk_info(disk_info_t *disk_info);
