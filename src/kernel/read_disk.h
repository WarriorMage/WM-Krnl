#pragma once

#include <stdint.h>
#include <stdbool.h>

bool read_sector(uint32_t lba, void *buf);
