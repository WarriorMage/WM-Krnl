#include <stdint.h>
#include <stddef.h>
#include "drivers/ata_driver.h"

#define SECTORS_PER_BLOCK 8

bool read_blocks(uint32_t block_number, uint8_t block_count, void *buf)
{
    return read_sectors(block_number * SECTORS_PER_BLOCK, block_count * SECTORS_PER_BLOCK, buf);
}

bool write_blocks(uint32_t block_number, uint8_t block_count, const void *buf)
{
    return write_sectors(block_number * SECTORS_PER_BLOCK, block_count * SECTORS_PER_BLOCK, buf);
}