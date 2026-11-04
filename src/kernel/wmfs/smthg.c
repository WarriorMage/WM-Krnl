#include "wmfs/smthg.h"
#include "drivers/keyboard.h"
#include "misc/utilities.h"
#include "wmfs/block.h"
#include "process/process.h"
#include "memory/vir_allocator.h"

#define SECTORS_PER_BLOCK 8
#define DATA_BLOCKS_PER_INODE 16
#define WMFS_MAGIC 0x574D4653

static superblock_t superblock;
uint8_t *ibitmap_cache, *dbitmap_cache;

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

bool initialize_superblock(void)
{
    disk_info_t disk_info;
    if (!get_disk_info(&disk_info))
        return false;
    get_block_counts(&disk_info, &superblock);
    superblock.ibitmap_start_block = 1;
    superblock.dbitmap_start_block = superblock.ibitmap_start_block + superblock.ibitmap_block_count;
    superblock.inode_start_block = superblock.dbitmap_start_block + superblock.dbitmap_block_count;
    superblock.data_start_block = superblock.inode_start_block + superblock.inode_block_count;
    // set the root inode as 1 for now
    superblock.root_inode_number = 1;
    superblock.wmfs_magic = WMFS_MAGIC; // means WMFS! haha! hahaha! hahahaha i am the greatest!!!
    return true;
}

typedef enum __bitmap_type
{
    INODE_BITMAP,
    DATA_BITMAP,
} bitmap_type;

bool fill_bitmap(bitmap_type type, uint32_t bit_number, alloc_bit alloc)
{
    uint8_t *chosen_bitmap;
    uint32_t start_block;
    switch (type)
    {
    case INODE_BITMAP:
        chosen_bitmap = ibitmap_cache;
        start_block = superblock.ibitmap_start_block;
        break;
    case DATA_BITMAP:
        chosen_bitmap = dbitmap_cache;
        start_block = superblock.dbitmap_start_block;
        break;
    }
    uint32_t block_offset = bit_number / (BLOCK_SIZE * 8);

    alloc_bit old_bit = bit_get(chosen_bitmap, bit_number);
    if (old_bit == alloc)
        return false;
    else
        bit_set(chosen_bitmap, bit_number, alloc);

    if (!write_blocks(start_block + block_offset, 1, chosen_bitmap + block_offset * BLOCK_SIZE))
    {
        bit_set(chosen_bitmap, bit_number, old_bit); // failure roll back
        return false;
    }
    return true;
}

// DANGEROUS! KNOW WHAT YOU ARE DOING FIRST!!!
bool init_disk_structures(void)
{
    char tmp_buf[BLOCK_SIZE] = {0};

    // WIPE THE DISK!
    for (size_t l = 0; l < superblock.ibitmap_block_count; ++l)
    {
        if (!write_blocks(superblock.ibitmap_start_block + l, 1, tmp_buf))
            return false;
        memcpy(ibitmap_cache + (l * BLOCK_SIZE), tmp_buf, BLOCK_SIZE);
    }
    for (size_t l = 0; l < superblock.dbitmap_block_count; ++l)
    {
        if (!write_blocks(superblock.dbitmap_start_block + l, 1, tmp_buf))
            return false;
        memcpy(dbitmap_cache + (l * BLOCK_SIZE), tmp_buf, BLOCK_SIZE);
    }

    // fill_bitmap(INODE_BITMAP, 1);
    // set_dbitmap(0);
    // duhh part

    memcpy(tmp_buf, &superblock, sizeof(superblock_t));
    if (!write_blocks(0, 1, tmp_buf))
        return false;
    return true;
}

bool init_bitmap_caches(void)
{
    ibitmap_cache = kmalloc(superblock.ibitmap_block_count * BLOCK_SIZE);
    dbitmap_cache = kmalloc(superblock.dbitmap_block_count * BLOCK_SIZE);
    if (!ibitmap_cache || !dbitmap_cache)
        return false;
    return true;
}

void format_wmfs(void)
{
    initialize_superblock();
    init_bitmap_caches();
    init_disk_structures();
}

void init_wmfs(void)
{
    char tmp_buf[BLOCK_SIZE];
    read_blocks(0, 1, tmp_buf);
    const superblock_t *read_superblock = (superblock_t *)tmp_buf;

    if (read_superblock->wmfs_magic != WMFS_MAGIC)
    {
        const char *format_message = "WM-Krnl requires a WMFS formatted disk, do you wish to format? (Y/y for yes, else no)";
        __sys_print_buffer_to_vga(&(syscall_args){0, 0, (uint32_t)format_message, 0x07, string_length(format_message), 0});
        char choice = getchar();

        if (choice != 'y' && choice != 'Y')
        {
            const char *sorry = "Disk not formatted, system halted. Remove the installation media and restart the computer.";
            __sys_print_buffer_to_vga(&(syscall_args){2, 0, (uint32_t)format_message, 0x07, string_length(format_message), 0});
            hard_stop();
        }
        format_wmfs();
    }
    else
    {
        memcpy(&superblock, read_superblock, sizeof(superblock_t));
        init_bitmap_caches();
        if (!read_blocks(superblock.ibitmap_start_block, superblock.ibitmap_block_count, ibitmap_cache) ||
            !read_blocks(superblock.dbitmap_start_block, superblock.dbitmap_block_count, dbitmap_cache))
            hard_stop(); // will do something better - placeholder
    }
}