#pragma once

#include <stddef.h>
#include "memory/phy_allocator.h"

extern uint32_t __kernel_end;
#define KHEAP_BASE 0xFE000000
#define KHEAP_END 0xFF000000

void *kmalloc(size_t size);
bool kfree(void *ptr);
