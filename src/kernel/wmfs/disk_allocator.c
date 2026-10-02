#include <stddef.h>
#include "wmfs/block.h"
#include "wmfs/smthg.h"
#include "memory/phy_allocator.h"

int32_t allocate_blocks(const superblock_t *superblock, uint32_t block_count)
{
    char buf[BLOCK_SIZE];
    uint32_t free_run = 0;
    bool found_run = false;
    size_t i = 0, j = 0;

    for (; i < superblock->dbitmap_block_count; ++i)
    {
        if (!read_blocks(superblock->dbitmap_start_block + i, 1, buf))
            return -1;
        for (; j < BLOCK_SIZE * 8; ++j)
        {
            if (bit_get(buf, j) == FREE)
                ++free_run;
            else
                free_run = 0;

            if (free_run == block_count)
            {
                found_run = true;
                // for (size_t k = 0; k < free_run; ++k)
                //     bit_set(buf, j - k, USED);
                // if (!write_blocks(superblock->dbitmap_start_block + i, 1, buf))
                //     return -1;

                // return superblock->data_start_block + (i * BLOCK_SIZE * 8 + j - block_count + 1);
            }
        }
    }
    if (!found_run)
        return -1;

    // floor((j - block_count + 1) / (BLOCK_SIZE * 8)); todo!();
}