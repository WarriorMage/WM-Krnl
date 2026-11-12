#include <stddef.h>
#include "block/block.h"
#include "fs/wmfs/block.h"
#include "fs/wmfs/init.h"
#include "fs/wmfs/inode.h"
#include "misc/utilities.h"

// calculates offset from the start block
uint32_t inode_to_block(uint32_t inode_num)
{
    return inode_num / INODES_PER_BLOCK;
}
// offset within the block
uint32_t inode_to_offset(uint32_t inode_num)
{
    return inode_num % INODES_PER_BLOCK;
}

#define INODE_CACHE_SIZE 32

typedef struct __icache_entry_t
{
    bool dirty;
    bool valid;
    size_t inode_number;
    inode_t cached_inode;
} icache_ent_t;

static struct
{
    icache_ent_t cache[INODE_CACHE_SIZE];
    uint8_t ptr, size;
} icache;

void icache_init(void)
{
    for (size_t i = 0; i < INODE_CACHE_SIZE; ++i)
        icache.cache[i].valid = false;
    icache.ptr = icache.size = 0;
}

void icache_evict(void)
{
    // don't do expensive evict when space exists in cache
    if (icache.size != INODE_CACHE_SIZE)
        return;
    icache_ent_t *evict_entry = &icache.cache[icache.ptr];

    if (evict_entry->dirty)
    {
        inode_t tmp_buf[INODES_PER_BLOCK];
        read_blocks(superblock.inode_start_block + inode_to_block(evict_entry->inode_number), 1, tmp_buf);
        tmp_buf[inode_to_offset(evict_entry->inode_number)] = evict_entry->cached_inode;
        write_blocks(superblock.inode_start_block + inode_to_block(evict_entry->inode_number), 1, tmp_buf);
    }
    --icache.size;
}

icache_ent_t *icache_insert(size_t inode_num, const inode_t *inode, bool dirty)
{
    if (inode_num == 0)
        return NULL;
    if (icache.size == INODE_CACHE_SIZE)
        icache_evict();

    icache_ent_t *insert_entry = &icache.cache[icache.ptr];
    insert_entry->cached_inode = *inode;
    insert_entry->inode_number = inode_num;
    insert_entry->dirty = dirty;
    insert_entry->valid = true;
    icache.ptr = (icache.ptr + 1) % INODE_CACHE_SIZE;
    ++icache.size;
    return insert_entry;
}

// O(n) linear scan
icache_ent_t *icache_scan(uint32_t inode_num)
{
    if (inode_num == 0)
        return NULL;
    for (size_t i = 0; i < INODE_CACHE_SIZE; ++i)
    {
        if (icache.cache[i].inode_number == inode_num && icache.cache[i].valid)
            return &icache.cache[i];
    }
    return NULL;
}

uint32_t ibitmap_alloc(void)
{
    static uint32_t next_check = 1;
    uint32_t i = next_check;
    do
    {
        if (i != 0 && bit_get(ibitmap_cache, i) == FREE)
        {
            bit_set(ibitmap_cache, i, USED);
            if (!write_blocks(superblock.ibitmap_start_block + bitmap_bit_to_block(i), 1, ibitmap_cache + bitmap_bit_to_block(i) * BLOCK_SIZE))
            {
                bit_set(ibitmap_cache, i, FREE);
                return 0;
            }
            next_check = (i + 1) % superblock.inode_count;
            return i;
        }
        i = (i + 1) % superblock.inode_count;
    } while (i != next_check);
    return 0;
}

bool ibitmap_free(uint32_t inode_num)
{
    if (inode_num == 0)
        return false;
    if (bit_get(ibitmap_cache, inode_num) == USED)
    {
        bit_set(ibitmap_cache, inode_num, FREE);
        if (!write_blocks(superblock.ibitmap_start_block + bitmap_bit_to_block(inode_num), 1, ibitmap_cache + bitmap_bit_to_block(inode_num) * BLOCK_SIZE))
        {
            bit_set(ibitmap_cache, inode_num, USED);
            return false;
        }
        return true;
    }
    return false;
}

uint32_t inode_create(bool root)
{
    uint32_t inode_num = root ? 1 : ibitmap_alloc();
    if (inode_num == 0)
        return 0;
    inode_t new_inode;
    new_inode.file_size = new_inode.block_count = new_inode.hard_link_count = 0;
    icache_insert(inode_num, &new_inode, true);
    return inode_num;
}

void inode_destroy(uint32_t inode_num)
{
    if (inode_num == 0)
        return;
    icache_ent_t *target_inode;
    if ((target_inode = icache_scan(inode_num)))
    {
        target_inode->dirty = false;
        target_inode->valid = false;
    }
    ibitmap_free(inode_num);
}

inode_t *inode_read(uint32_t inode_num)
{
    if (inode_num == 0)
        return NULL;
    icache_ent_t *target_inode;
    if ((target_inode = icache_scan(inode_num)))
        return &target_inode->cached_inode;

    inode_t tmp_buf[INODES_PER_BLOCK];
    read_blocks(superblock.inode_start_block + inode_to_block(inode_num), 1, tmp_buf);
    return &icache_insert(inode_num, &tmp_buf[inode_to_offset(inode_num)], false)->cached_inode;
}

void inode_write(uint32_t inode_num, const inode_t *data)
{
    if (inode_num == 0)
        return;
    icache_ent_t *target_inode;
    if ((target_inode = icache_scan(inode_num)))
    {
        target_inode->dirty = true;
        target_inode->cached_inode = *data;
    }
    else
    {
        icache_insert(inode_num, data, true);
    }
}