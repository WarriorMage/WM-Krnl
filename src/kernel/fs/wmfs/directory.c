#include <stdint.h>
#include "misc/utilities.h"
#include "block/block.h"
#include "fs/wmfs/block.h"
#include "fs/wmfs/init.h"
#include "fs/wmfs/inode.h"

typedef struct __dirent_t
{
    uint32_t inode_number;
    uint32_t entry_len;
    uint32_t name_len;
    char name[];
} dirent_t;

#define ROOT_INODE 1

uint32_t directory_search(uint32_t inode_num, const char *target)
{
    inode_t *dir_inode = inode_read(inode_num);
    char read_buf[BLOCK_SIZE];
    for (size_t i = 0; i < dir_inode->block_count; ++i)
    {
        size_t read_size = 0;
        read_blocks(dir_inode->data_blocks[i], 1, read_buf);
        const dirent_t *dir_entry = (const dirent_t *)read_buf;

        while (read_size <= BLOCK_SIZE - sizeof(dirent_t))
        {
            if (dir_entry->inode_number != 0 && string_length(target) == dir_entry->name_len && strncmp(dir_entry->name, target, dir_entry->name_len) == 0)
                return dir_entry->inode_number;

            read_size += dir_entry->entry_len;
            dir_entry = (const dirent_t *)((const char *)dir_entry + dir_entry->entry_len);
        }
    }
    return 0;
}

#define NAME_MAX 255

typedef struct __trav_state
{
    uint16_t idx;
    uint32_t cur_inode;
} trav_state;

void trav_state_init(trav_state *state)
{
    state->idx = 0;
    state->cur_inode = ROOT_INODE;
}

uint32_t filepath_parser(const char *path, trav_state *state)
{
    uint32_t cur_inode = state->cur_inode;
    const char *path_trav = path + state->idx;
    char name[NAME_MAX + 1];

    uint8_t idx = 0;
    if (*path_trav == '/')
    {
        ++path_trav;
        state->idx += 1;
    }
    while (*path_trav != '/' && idx < NAME_MAX)
        name[idx++] = *(path_trav++);
    name[idx] = '\0';
    state->idx += idx;

    if (idx > 0)
        state->cur_inode = directory_search(cur_inode, name);
    return state->cur_inode;
}

void make_dir(bool root)
{
    uint32_t inode_num = inode_create(root);
    if (inode_num == 0)
        return;
    uint32_t dblock = dbitmap_alloc();
    if (dblock == 0)
        return;
    inode_t *dir_inode = inode_read(inode_num); // inode num is non zero
    // just creating, block count guaranteed to be zero.
    dir_inode->data_blocks[dir_inode->block_count++] = dblock;
    inode_write(inode_num, dir_inode);

    char read_buf[BLOCK_SIZE];
    read_blocks(dblock, 1, read_buf);
    write_blocks(dblock, 1, read_buf);
}