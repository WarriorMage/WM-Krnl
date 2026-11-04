#include <stdint.h>
#include <stddef.h>
#include "wmfs/block.h"
#include "drivers/ata_driver.h"

#define SECTORS_PER_BLOCK 8

bool read_blocks(uint32_t block_number, uint8_t block_count, void *buf)
{
    return ata_read_sectors(block_number * SECTORS_PER_BLOCK, block_count * SECTORS_PER_BLOCK, buf);
}

bool write_blocks(uint32_t block_number, uint8_t block_count, const void *buf)
{
    return ata_write_sectors(block_number * SECTORS_PER_BLOCK, block_count * SECTORS_PER_BLOCK, buf);
}

bool get_disk_info(disk_info_t *disk_info)
{
    ata_disk_info_t ata_disk_info;
    if (!ata_fill_disk_info(&ata_disk_info))
        return false;
    disk_info->sector_count = ata_disk_info.sector_count;
    return true;
}
