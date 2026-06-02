#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "paging.h"
#include "vir_allocator.h"

// Should return to fix alignment, and returning pages to OS, request of multiple pages.

// For sanity check, shouldn't end up 'freeing' up stack space
#define MAGIC_NUMBER 12345
// If we allocated only 4 bytes total block size = 16(meta) + 4 = 20, now free node requires 24 bytes so can't free
#define MIN_ALLOC (sizeof(free_node) - sizeof(block_header)) // So that the block can be converted to a free node

typedef struct free_node
{
    size_t length;
    struct free_node *prev;
    struct free_node *next;
} free_node;

static free_node *head = NULL; // Local to this file

typedef struct block_header
{
    size_t size;
    uint16_t magic_number;
} block_header;

free_node *find_chunk(size_t size)
{
    // First fit
    free_node *trav = head;
    while (trav)
    {
        if (trav->length >= size)
            return trav;
        trav = trav->next;
    }
    return trav;
}

void copy_free_node(const free_node *src, free_node *dest)
{
    dest->length = src->length;
    dest->next = src->next;
    dest->prev = src->prev;
}

void free_list_insert(free_node *insert_node)
{
    if (!insert_node) // Not possible since everywhere we called we ensured that arg is valid
        return;       // but still for safety ig

    free_node *trav = head;
    while (trav && trav->next && trav < insert_node) // trav != insert_node we are just freeing insert_node
        trav = trav->next;

    if (!trav || trav < insert_node) // There should be no free list entry > insert_node
    {
        insert_node->prev = trav;
        insert_node->next = NULL;
        if (trav)
            trav->next = insert_node;
        else
            head = insert_node;
    }
    else
    {
        insert_node->next = trav;
        insert_node->prev = trav->prev;
        if (trav->prev)
            trav->prev->next = insert_node;
        else
            head = insert_node;
        trav->prev = insert_node;
    }
}

void split(free_node *original_chunk, size_t size)
{
    free_node *split_chunk = (free_node *)((char *)original_chunk + sizeof(block_header) + size);
    copy_free_node(original_chunk, split_chunk);

    split_chunk->length -= (sizeof(block_header) + size);
    if (split_chunk->prev)
        split_chunk->prev->next = split_chunk;
    else // No previous node, should be head
        head = split_chunk;
    if (split_chunk->next)
        split_chunk->next->prev = split_chunk;
}

void coalesce(free_node **new_free_node_ptr)
{
    free_node *new_free_node = *new_free_node_ptr; // Passed by reference as new_free_node can change
                                                   // after coalesce

    if (new_free_node->next && new_free_node->next == (free_node *)((char *)new_free_node + new_free_node->length))
    {
        new_free_node->length = new_free_node->length + new_free_node->next->length;
        new_free_node->next = new_free_node->next->next;
        // if (new_free_node->next) ; redundant
            new_free_node->next->prev = new_free_node;
    }
    if (new_free_node->prev && new_free_node->prev == (free_node *)((char *)new_free_node - new_free_node->prev->length))
    {
        new_free_node = new_free_node->prev;
        new_free_node->length = new_free_node->length + new_free_node->next->length;
        new_free_node->next = new_free_node->next->next;
        if (new_free_node->next)
            new_free_node->next->prev = new_free_node;
    }
    *new_free_node_ptr = new_free_node;
}

bool add_new_page(void)
{
    static size_t heap_page_number = 0;
    // why am i protecting just the stack top instead of the entire region? fix this
    if (heap_page_number >= ((STACK_BASE - HEAP_BASE) / PAGE_SIZE))
        return false;

    uint32_t heap_region = HEAP_BASE + heap_page_number * PAGE_SIZE;
    if (!map_page_to_frame(PAGE_DIRECTORY_ADDR, heap_region))
        return false;
    
    heap_page_number++;
    free_node *new_region = (void *)heap_region;
    new_region->length = PAGE_SIZE;

    free_list_insert(new_region);
    coalesce(&new_region);
    if (!new_region->prev)
        head = new_region;

    return true;
}

void *kmalloc(size_t size)
{
    if (size < MIN_ALLOC)
        size = MIN_ALLOC;

    free_node *chunk = find_chunk(size + sizeof(block_header));
    while (!chunk) // while handles requests for objects that need multiple pages, omg so big
    {
        if (!add_new_page())
            return NULL;
        chunk = find_chunk(size + sizeof(block_header));
    }

    size_t split_space = chunk->length - (size + sizeof(block_header));
    if (split_space >= sizeof(free_node))
        split(chunk, size);

    else // Here we consume the whole free node instead of splitting since the split space can't hold
    {    // a free node, very small
        if (chunk->prev)
            chunk->prev->next = chunk->next;
        else
            head = chunk->next; // If the first node is consumed make the next one head.
        if (chunk->next)
            chunk->next->prev = chunk->prev;
    }

    block_header *allocated_chunk = (block_header *)chunk;
    allocated_chunk->size = size + ((split_space < sizeof(free_node)) ? split_space : 0);
    allocated_chunk->magic_number = MAGIC_NUMBER;

    void *return_block = (void *)((char *)allocated_chunk + sizeof(block_header));
    return return_block;
}

bool kfree(void *ptr)
{
    if (!ptr || !head)
        return false;

    block_header *header = (block_header *)ptr - 1;
    if (header->magic_number != MAGIC_NUMBER)
        return false; // Maybe not heap memory
    size_t block_size = sizeof(block_header) + header->size;

    free_node *new_free_node = (free_node *)header;
    new_free_node->length = block_size;

    free_list_insert(new_free_node); // Updates next and prevs appropriately
    coalesce(&new_free_node);
    return true;
}
