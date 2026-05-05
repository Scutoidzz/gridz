#include <stddef.h>
#include <stdint.h>

void mp_embed_init(void *gc_heap, size_t gc_heap_size, void *stack_top) {
}

void mp_embed_deinit(void) {
}

void mp_embed_exec_str(const char *src) {
    extern void python_term_write(const char *str, size_t len);

    python_term_write("error: MicroPython not yet integrated\n", 37);
    if (src) {
        const char *start = src;
        const char *end = start;
        while (*end) end++;
        python_term_write("input: ", 7);
        python_term_write(start, end - start);
        python_term_write("\n", 1);
    }
}
