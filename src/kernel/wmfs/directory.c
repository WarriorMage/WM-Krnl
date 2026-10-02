#include <stdint.h>
#include "wmfs/block.h"
#include "wmfs/smthg.h"

typedef struct __dirent_t
{
    uint32_t inode_number;
    uint32_t entry_len;
    uint32_t name_len;
    char name[];
} dirent_t;

bool write_dir_entry_inode(const inode_t *dir_inode, uint32_t inode_number, const char *name)
{
    char buf[BLOCK_SIZE];
    if(dir_inode->file_size == 0)
        allocate_blocks(1);
    uint32_t dir_block = dir_inode->data_blocks[0].start_block;
    if (!read_blocks(dir_block, 1, buf))
        return false;
    return true;
}