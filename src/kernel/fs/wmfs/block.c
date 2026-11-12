#include <stdint.h>
#include "misc/utilities.h"
#include "fs/wmfs/init.h"
#include "fs/wmfs/block.h"
#include "block/block.h"
#include "drivers/ata_driver.h"

// calculates offset from start block
uint32_t bitmap_bit_to_block(uint32_t bit)
{
    return bit / (BLOCK_SIZE * 8);
}

// Returns the absolute allocated block number
uint32_t dbitmap_alloc(void)
{
    static uint32_t next_check = 0;
    uint32_t i = next_check;
    do
    {
        if (bit_get(dbitmap_cache, i) == FREE)
        {
            bit_set(dbitmap_cache, i, USED);
            if (!write_blocks(superblock.dbitmap_start_block + bitmap_bit_to_block(i), 1, dbitmap_cache + bitmap_bit_to_block(i) * BLOCK_SIZE))
            {
                bit_set(dbitmap_cache, i, FREE);
                return 0;
            }
            next_check = (i + 1) % superblock.data_block_count;
            return superblock.data_start_block + i;
        }
        i = (i + 1) % superblock.data_block_count;
    } while (i != next_check);
    return 0;
}

// Function takes in absolute block number
bool dbitmap_free(uint32_t block_num)
{
    if (block_num < superblock.data_start_block || block_num >= superblock.data_start_block + superblock.data_block_count)
        return false;
    block_num -= superblock.data_start_block;

    if (bit_get(dbitmap_cache, block_num) == USED)
    {
        bit_set(dbitmap_cache, block_num, FREE);
        if (!write_blocks(superblock.dbitmap_start_block + bitmap_bit_to_block(block_num), 1, dbitmap_cache + bitmap_bit_to_block(block_num) * BLOCK_SIZE))
        {
            bit_set(dbitmap_cache, block_num, USED);
            return false;
        }
        return true;
    }
    return false;
}
