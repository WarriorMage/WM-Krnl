#pragma once

#include <stdint.h>

typedef struct __superblock_t_
{
    // format : superblock | ibitmap | dbitmap | iblocks | dblocks
    uint32_t wmfs_magic;
    uint32_t root_inode_number;

    uint32_t ibitmap_start_block;
    uint32_t ibitmap_block_count;

    uint32_t dbitmap_start_block;
    uint32_t dbitmap_block_count;

    uint32_t inode_start_block;
    uint32_t inode_block_count;

    uint32_t data_start_block;
    uint32_t data_block_count;

} superblock_t;

typedef struct __extent_t
{
    uint32_t start_block;
    uint32_t block_count;
} extent_t;

typedef struct __inode_t
{
    uint32_t file_size;
    extent_t data_blocks[7];
    uint16_t hard_link_count;
} inode_t;
