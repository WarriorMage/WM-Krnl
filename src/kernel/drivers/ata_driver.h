#pragma once

#include <stdint.h>
#include <stdbool.h>

bool read_sectors(uint32_t lba, uint8_t sec_count, void *buf);
bool write_sectors(uint32_t lba, uint8_t sec_count, const void *buf);
