#include "wmfs/smthg.h"
#include "drivers/ata_driver.h"
#include "misc/utilities.h"
#include "wmfs/block.h"

#define SECTORS_PER_BLOCK 8
#define DATA_BLOCKS_PER_INODE 16

superblock_t disk_superblock;

// inode size = 64 bytes, inodes per block = 4096 / 64 = 64
// 1 inode / 16 data blocks => data_blocks data blocks = data_blocks / 16 inodes = data_blocks data
// blocks / 16 * 64 inode blocks
// total_blocks = data_blocks + ceil(data_blocks / 1024) + ceil(data_blocks / 32768) + ceil(ceil(data_blocks / 16) / 32768) + 1 ; order in formulae: data blocks, inode blocks, data bitmap, inode bitmap, superblock

static uint32_t ceil_div(uint32_t top, uint32_t bottom)
{
    return (top / bottom) + ((top % bottom) ? 1 : 0);
}

void get_block_counts(const disk_info_t *disk_info, superblock_t *superblock)
{
    const uint32_t total_blocks = disk_info->sector_count / SECTORS_PER_BLOCK;
    // idc about the last maybe 7 sectors
    uint32_t data_blocks = total_blocks;

    while (true) // this only will run twice anyways
    {
        superblock->data_block_count = data_blocks; // assume all data blocks
        superblock->inode_block_count = ceil_div(data_blocks, DATA_BLOCKS_PER_INODE * (BLOCK_SIZE / sizeof(inode_t)));
        superblock->dbitmap_block_count = ceil_div(data_blocks, BLOCK_SIZE * 8); // get in bits
        superblock->ibitmap_block_count = ceil_div(ceil_div(data_blocks, DATA_BLOCKS_PER_INODE), BLOCK_SIZE * 8);

        uint32_t calc_total_blocks = superblock->data_block_count + superblock->inode_block_count + superblock->dbitmap_block_count + superblock->ibitmap_block_count + 1;

        if (calc_total_blocks > total_blocks)
            data_blocks -= (calc_total_blocks - total_blocks);
        else
            break;
    }
}

bool initialize_superblock(superblock_t *superblock)
{
    disk_info_t disk_info;
    if (!fill_disk_info(&disk_info))
        return false;
    get_block_counts(&disk_info, superblock);
    superblock->ibitmap_start_block = 1;
    superblock->dbitmap_start_block = superblock->ibitmap_start_block + superblock->ibitmap_block_count;
    superblock->inode_start_block = superblock->dbitmap_start_block + superblock->dbitmap_block_count;
    superblock->data_start_block = superblock->inode_start_block + superblock->inode_block_count;
    // set the root inode as 1 for now
    superblock->root_inode_number = 1;
    superblock->wmfs_magic = 0x574D4653; // means WMFS! haha! hahaha! hahahaha i am the greatest!!!
    return true;
}


// DANGEROUS! KNOW WHAT YOU ARE DOING FIRST!!!
bool init_disk_structures(const superblock_t *superblock)
{
    char tmp_buf[BLOCK_SIZE] = {0};

    size_t metadata_block_count = superblock->ibitmap_block_count + superblock->dbitmap_block_count + superblock->inode_block_count;

    // WIPE THE DISK!
    for (size_t l = 0; l < metadata_block_count; ++l)
    {
        if (!write_blocks(superblock->ibitmap_start_block + l, 1, tmp_buf))
            return false;
    }

    // set_ibitmap(1);
    // set_dbitmap(0);
    // duhh part

    memcpy(tmp_buf, superblock, sizeof(superblock_t));
    if (!write_blocks(0, 1, tmp_buf))
        return false;
}