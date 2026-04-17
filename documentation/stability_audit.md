# Gridz Stability Audit & Roadmap

## Critical Fixes Required

### 1. Dynamic Memory Allocation
The current implementation of `malloc` is a simple bump allocator:
```cpp
void* malloc(size_t size) {
    if (heap_ptr + size > HEAP_SIZE) return NULL;
    void* ptr = &heap[heap_ptr];
    heap_ptr += (size + 15) & ~15;
    return ptr;
}
```
**Risk**: High. Memory is never reclaimed, leading to inevitable crashes in long-running processes like Doom.
**Recommendation**: Implement a Linked List allocator where each block has a header:
```cpp
struct HeapBlock {
    size_t size;
    bool is_free;
    HeapBlock* next;
};
```

### 2. Terminal Scrolling Logic
The nested loop in `terminal.hpp` is an $O(N)$ operation on total pixels.
```cpp
for (int y = 0; y < (int)fb->height - 8; y++) {
    for (int x = 0; x < (int)fb->width; x++) {
        fb_ptr[y * stride + x] = fb_ptr[(y + 8) * stride + x];
    }
}
```
**Risk**: Performance bottleneck. Screen clear and scroll will stutter on larger displays.
**Recommendation**: Use `memmove` for bulk transfers. The CPU can handle large memory copies much faster than per-pixel assignments.

### 3. Interrupt Safety
The keyboard queue in `main.cpp` is written by an ISR and read by the main loop.
**Risk**: Data corruption. If an interrupt fires while the main loop is mid-calculation of `key_tail`, the state may become inconsistent.
**Recommendation**: Use `volatile` correctly (already done for some variables) and disable interrupts (`cli`/`sti`) during queue read operations.

### 4. Doom Blocking behavior
The `doom` command enters a `while(1)` loop in `execute_command`.
**Risk**: Total system hang. You cannot exit Doom or return to the shell.
**Recommendation**: Implement a way to detect the `ESC` key or a special combo to break the loop, or better yet, implement basic cooperative multitasking.

## Suggested Development Order
1. **Fix Memory**: Implement `HeapBlock` based allocator.
2. **Optimize UI**: Switch to `memmove` for scrolling.
3. **Idle Loop**: Add `hlt` to the kernel's main loop to save power.
4. **Multitasking**: Create a simple context switching mechanism to run the shell and doom as separate tasks.
