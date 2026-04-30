#pragma once

#include <stdint.h>
#include <stddef.h>

// Initialize the Physical Memory Manager using the Limine memory map.
void pmm_init();

// Allocate contiguous 4KB pages and return the physical address.
void* pmm_alloc_pages(size_t pages);

// Free contiguous 4KB pages given the physical address.
void pmm_free_pages(void* ptr, size_t pages);
