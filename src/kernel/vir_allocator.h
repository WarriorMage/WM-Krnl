#pragma once

#include <stddef.h>
#include "phy_allocator.h"

extern uint32_t __kernel_end;
#define HEAP_BASE align_up((uint32_t) &__kernel_end)

void *kmalloc(size_t size);
bool kfree(void *ptr);
