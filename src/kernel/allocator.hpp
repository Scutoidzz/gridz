#pragma once

#include <stdint.h>
#include <stddef.h>
void pmm_init();
void* pmm_alloc_pages(size_t pages);
void pmm_free_pages(void* ptr, size_t pages);
