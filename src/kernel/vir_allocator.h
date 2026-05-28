#pragma once

#include <stddef.h>

void *kmalloc(size_t size);
bool kfree(void *ptr);
