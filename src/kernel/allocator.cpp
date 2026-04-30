#include "allocator.hpp"
#include "limine.h"

// Defined in main.cpp
extern volatile struct limine_memmap_request memmap_request;
extern uint64_t hhdm_offset;
extern "C" void serial_print(const char* s);


static uint8_t* pmm_bitmap = nullptr;
static uint64_t pmm_bitmap_size = 0; // in bytes
static uint64_t pmm_total_pages = 0;
static uint64_t pmm_free_pages_count = 0;

static void pmm_bitmap_set(uint64_t bit) {
    pmm_bitmap[bit / 8] |= (1 << (bit % 8));
}

static void pmm_bitmap_clear(uint64_t bit) {
    pmm_bitmap[bit / 8] &= ~(1 << (bit % 8));
}

static bool pmm_bitmap_test(uint64_t bit) {
    return (pmm_bitmap[bit / 8] & (1 << (bit % 8))) != 0;
}

void pmm_init() {
    struct limine_memmap_response* memmap = memmap_request.response;
    if (!memmap) return;

    uint64_t highest_addr = 0;
    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry* entry = memmap->entries[i];
        if (entry->type == LIMINE_MEMMAP_USABLE ||
            entry->type == LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE ||
            entry->type == LIMINE_MEMMAP_ACPI_RECLAIMABLE) {
            uint64_t top = entry->base + entry->length;
            if (top > highest_addr) highest_addr = top;
        }
    }

    pmm_total_pages = highest_addr / 4096;
    pmm_bitmap_size = pmm_total_pages / 8;
    if (pmm_total_pages % 8 != 0) pmm_bitmap_size++;

    // Find a contiguous block of usable memory to place the bitmap itself
    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry* entry = memmap->entries[i];
        if (entry->type == LIMINE_MEMMAP_USABLE && entry->length >= pmm_bitmap_size) {
            // Place it here! (accessed virtually via HHDM)
            pmm_bitmap = (uint8_t*)(entry->base + hhdm_offset);
            
            // Mark all memory as used (1) initially
            for (uint64_t j = 0; j < pmm_bitmap_size; j++) {
                pmm_bitmap[j] = 0xFF;
            }
            
            // Exclude the bitmap itself from the usable memory region
            entry->base += pmm_bitmap_size;
            entry->length -= pmm_bitmap_size;
            break;
        }
    }

    if (!pmm_bitmap) {
        serial_print("PMM ERROR: Could not find space for bitmap!\n");
        return;
    }

    // Now loop again and mark the truly usable areas as free (0)
    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry* entry = memmap->entries[i];
        if (entry->type == LIMINE_MEMMAP_USABLE ||
            entry->type == LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE ||
            entry->type == LIMINE_MEMMAP_ACPI_RECLAIMABLE) {
            
            uint64_t start = entry->base / 4096;
            uint64_t end = (entry->base + entry->length) / 4096;
            for (uint64_t j = start; j < end; j++) {
                if (j == 0) continue; // Never allocate physical page 0 (avoids NULL pointer issues)
                pmm_bitmap_clear(j);
                pmm_free_pages_count++;
            }
        }
    }
    
    serial_print("PMM Initialized successfully.\n");
}

void* pmm_alloc_pages(size_t pages) {
    if (pages == 0) return nullptr;
    
    size_t contiguous = 0;
    uint64_t start_bit = 0;
    
    // Simple first-fit search
    for (uint64_t i = 0; i < pmm_total_pages; i++) {
        if (!pmm_bitmap_test(i)) {
            if (contiguous == 0) start_bit = i;
            contiguous++;
            if (contiguous == pages) {
                // Found enough pages!
                for (size_t j = 0; j < pages; j++) {
                    pmm_bitmap_set(start_bit + j);
                }
                pmm_free_pages_count -= pages;
                return (void*)(start_bit * 4096);
            }
        } else {
            contiguous = 0;
        }
    }
    
    serial_print("PMM ERROR: Out of physical memory!\n");
    return nullptr; // Out of memory
}

void pmm_free_pages(void* ptr, size_t pages) {
    uint64_t start_bit = (uint64_t)ptr / 4096;
    for (size_t i = 0; i < pages; i++) {
        pmm_bitmap_clear(start_bit + i);
    }
    pmm_free_pages_count += pages;
}

// ─── liballoc hooks ───────────────────────────────────────────────────────────
// These are called internally by liballoc to manage heap memory

extern "C" {

int liballoc_lock() {
    // Return 0 on success. We don't have SMP or threads right now,
    // so no need for an actual spinlock yet.
    return 0;
}

int liballoc_unlock() {
    return 0;
}

void* liballoc_alloc(int pages) {
    // liballoc asks for `pages` (4KB each). We allocate physical pages...
    void* phys = pmm_alloc_pages(pages);
    if (!phys) return nullptr;
    
    // ...and give liballoc the higher-half virtual address where it can access it.
    return (void*)((uint64_t)phys + hhdm_offset);
}

int liballoc_free(void* ptr, int pages) {
    // `ptr` is a higher-half virtual address. We convert back to physical.
    void* phys = (void*)((uint64_t)ptr - hhdm_offset);
    pmm_free_pages(phys, pages);
    return 0;
}

}
