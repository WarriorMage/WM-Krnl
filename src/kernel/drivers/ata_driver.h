#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "wmfs/smthg.h"

typedef struct __disk_info_t
{
    uint64_t sector_count; // disk size in sectors
    // may add more fields in future?
} disk_info_t;

bool read_sectors(uint32_t lba, uint8_t sec_count, void *buf);
bool write_sectors(uint32_t lba, uint8_t sec_count, const void *buf);
bool fill_disk_info(disk_info_t *disk_info);
