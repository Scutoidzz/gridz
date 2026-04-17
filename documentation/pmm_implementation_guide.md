# PMM Implementation Guide

To implement the Physical Memory Manager, follow these concrete steps:

## 1. The PMM Class (pmm.hpp)
Define a class that manages the bitmap. You need to keep track of the bitmap's address, its size, and the total number of pages.

```cpp
class PMM {
    uint8_t* bitmap;
    uint64_t total_pages;
    uint64_t bitmap_size;
public:
    void init(struct sboot_memmap_response* memmap, uint64_t hhdm);
    void* alloc_page();
    void free_page(void* addr);
};
```

## 2. Initializing the Bitmap
1.  **Iterate once** to find the highest memory address to determine `total_pages`.
2.  **Calculate `bitmap_size`** (`total_pages / 8`).
3.  **Find a USABLE region** large enough to hold the bitmap.
4.  **Mark everything as USED** initially to be safe.
5.  **Iterate again** and mark all `sboot_MEMMAP_USABLE` pages as FREE.
6.  **Re-mark** the bitmap's own pages and the kernel's pages as USED.

## 3. Allocation Logic
A simple bit-scan:
```cpp
void* alloc_page() {
    for (uint64_t i = 0; i < total_pages; i++) {
        if (!get_bit(i)) {
            set_bit(i, true);
            return (void*)(i * 4096);
        }
    }
    return nullptr;
}
```

## 4. Helper Macros
You will need helpers to manipulate bits in the `uint8_t` array:
- `idx = page / 8`
- `bit = page % 8`
- `bitmap[idx] |= (1 << bit)` // Set
- `bitmap[idx] &= ~(1 << bit)` // Unset
- `bitmap[idx] & (1 << bit)` // Test
