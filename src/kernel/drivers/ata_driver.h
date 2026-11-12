#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct __ata_disk_info_t
{
    uint64_t sector_count; // disk size in sectors
    // may add more fields in future?
} ata_disk_info_t;

bool ata_read_sectors(uint32_t lba, uint8_t sec_count, void *buf);
bool ata_write_sectors(uint32_t lba, uint8_t sec_count, const void *buf);
bool ata_fill_disk_info(ata_disk_info_t *disk_info);
