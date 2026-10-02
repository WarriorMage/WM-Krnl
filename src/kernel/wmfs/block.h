#pragma once

#include <stdint.h>
#include <stdbool.h>
#define BLOCK_SIZE 4096

bool read_blocks(uint32_t block_number, uint8_t block_count, void *buf);
bool write_blocks(uint32_t block_number, uint8_t block_count, const void *buf);
