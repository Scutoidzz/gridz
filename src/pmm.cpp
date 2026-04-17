#include "pmm.hpp"
#include "terminal.hpp"

uint8_t* PMM::bitmap = nullptr;
uint64_t PMM::bitmap_size = 0;
uint64_t PMM::total_pages = 0;
uint64_t PMM::hhdm_offset = 0;

extern Terminal* global_term;

void PMM::init(struct sboot_memmap_response* memmap, struct sboot_hhdm_response* hhdm) {
    hhdm_offset = hhdm->offset;
    uint64_t highest_address = 0;

    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        struct sboot_memmap_entry* entry = memmap->entries[i];
        if (entry->base + entry->length > highest_address) {
            highest_address = entry->base + entry->length;
        }
    }

    total_pages = highest_address / 4096;
    bitmap_size = (total_pages + 7) / 8;

    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        struct sboot_memmap_entry* entry = memmap->entries[i];
        if (entry->type == sboot_MEMMAP_USABLE && entry->length >= bitmap_size) {
            bitmap = (uint8_t*)(entry->base + hhdm_offset);
            entry->base += (bitmap_size + 4095) & ~4095;
            entry->length -= (bitmap_size + 4095) & ~4095;
            break;
        }
    }

    // Initialize bitmap: all used
    for (uint64_t i = 0; i < bitmap_size; i++) {
        bitmap[i] = 0xFF;
    }

    // Mark usable regions as free
    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        struct sboot_memmap_entry* entry = memmap->entries[i];
        if (entry->type == sboot_MEMMAP_USABLE) {
            for (uint64_t addr = entry->base; addr < entry->base + entry->length; addr += 4096) {
                clear_bit(addr / 4096);
            }
        }
    }
}

void PMM::set_bit(uint64_t bit) {
    bitmap[bit / 8] |= (1 << (bit % 8));
}

void PMM::clear_bit(uint64_t bit) {
    bitmap[bit / 8] &= ~(1 << (bit % 8));
}

bool PMM::get_bit(uint64_t bit) {
    return (bitmap[bit / 8] >> (bit % 8)) & 1;
}

void* PMM::alloc_page() {
    return alloc_pages(1);
}

void* PMM::alloc_pages(uint64_t count) {
    for (uint64_t i = 0; i < total_pages - count; i++) {
        bool found = true;
        for (uint64_t j = 0; j < count; j++) {
            if (get_bit(i + j)) {
                found = false;
                i += j; // Skip checked bits
                break;
            }
        }
        if (found) {
            for (uint64_t j = 0; j < count; j++) set_bit(i + j);
            return (void*)(i * 4096);
        }
    }
    return nullptr;
}

void PMM::free_page(void* ptr) {
    uint64_t bit = (uint64_t)ptr / 4096;
    clear_bit(bit);
}
