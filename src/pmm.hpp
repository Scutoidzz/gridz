#pragma once
#include <stdint.h>
#include <stddef.h>
#include "sboot.h"

class PMM {
public:
    static void init(struct sboot_memmap_response* memmap, struct sboot_hhdm_response* hhdm);
    static void* alloc_page();
    static void* alloc_pages(uint64_t count);
    static void free_page(void* ptr);

private:
    static uint8_t* bitmap;
    static uint64_t bitmap_size;
    static uint64_t total_pages;
    static uint64_t hhdm_offset;

    static void set_bit(uint64_t bit);
    static void clear_bit(uint64_t bit);
    static bool get_bit(uint64_t bit);
};
