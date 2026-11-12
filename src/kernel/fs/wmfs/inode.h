#pragma once

#include <stdbool.h>

extern uint8_t *ibitmap_cache;

typedef struct __inode_t
{
    uint32_t file_size;
    uint32_t data_blocks[14];
    uint8_t block_count;
    uint16_t hard_link_count;
} inode_t;

#define INODES_PER_BLOCK (BLOCK_SIZE / sizeof(inode_t))

uint32_t inode_create(bool root);
void inode_destroy(uint32_t inode_num);
inode_t *inode_read(uint32_t inode_num);
void inode_write(uint32_t inode_num, const inode_t *data);
