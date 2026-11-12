#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct __superblock_t_
{
    // format : superblock | ibitmap | dbitmap | iblocks | dblocks
    uint32_t wmfs_magic;
    uint32_t root_inode_number;

    uint32_t ibitmap_start_block;
    uint32_t ibitmap_block_count;
    uint32_t inode_count;

    uint32_t dbitmap_start_block;
    uint32_t dbitmap_block_count;

    uint32_t inode_start_block;
    uint32_t inode_block_count;

    uint32_t data_start_block;
    uint32_t data_block_count;

} superblock_t;

extern superblock_t superblock;
