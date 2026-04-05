#pragma once

#include <stdint.h>

typedef uint32_t page_directory_entry;
page_directory_entry *setup_page_directory(void);
void init_paging(page_directory_entry *directory_address);
