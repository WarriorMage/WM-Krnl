#pragma once

typedef struct __inode_t
{
    uint32_t file_size;
    extent_t data_blocks[7];
    uint8_t extent_count;
    uint16_t hard_link_count;
} inode_t;

#define INODES_PER_BLOCK (BLOCK_SIZE / sizeof(inode_t))
