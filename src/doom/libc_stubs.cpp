#include "libc_gridz.h"
#include <stdint.h>
#include "../../terminal.hpp"
#include "../limine.h"

extern Terminal* global_term;
extern "C" void serial_print(const char* s);
extern "C" void itoa(int n, char* s, int base);

extern "C" {

struct limine_module_response* get_module_response();

struct FILE {
    uint8_t* data;
    size_t size;
    size_t pos;
};

#define HEAP_SIZE (1024 * 1024 * 64)
static uint8_t heap[HEAP_SIZE];
static size_t heap_ptr = 0;

struct malloc_tag {
    size_t size;
};

void* malloc(size_t size) {
    size_t total = size + sizeof(malloc_tag);
    if (heap_ptr + total > HEAP_SIZE) {
        serial_print("MALLOC FAILED: requested ");
        char buf[20]; itoa(size, buf, 10); serial_print(buf);
        serial_print("\n");
        return NULL;
    }
    malloc_tag* tag = (malloc_tag*)&heap[heap_ptr];
    tag->size = size;
    void* ptr = (void*)(tag + 1);
    heap_ptr += (total + 15) & ~15;
    return ptr;
}

void* calloc(size_t nmemb, size_t size) {
    size_t total = nmemb * size;
    void* ptr = malloc(total);
    if (ptr) memset(ptr, 0, total);
    return ptr;
}

void free(void* ptr) { (void)ptr; }

void* realloc(void* ptr, size_t size) {
    if (!ptr) return malloc(size);
    if (size == 0) { free(ptr); return NULL; }
    
    malloc_tag* tag = ((malloc_tag*)ptr) - 1;
    size_t old_size = tag->size;
    
    if (size <= old_size) {
        tag->size = size; // Shrink in place (simple)
        return ptr;
    }

    void* new_ptr = malloc(size);
    if (!new_ptr) return NULL;
    
    memcpy(new_ptr, ptr, old_size);
    return new_ptr;
}


char* strdup(const char* s) {
    size_t len = strlen(s);
    char* d = (char*)malloc(len + 1);
    memcpy(d, s, len + 1);
    return d;
}

int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) { s1++; s2++; }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

int strcasecmp(const char* s1, const char* s2) {
    while (*s1 && (tolower(*s1) == tolower(*s2))) { s1++; s2++; }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

int strncasecmp(const char* s1, const char* s2, size_t n) {
    if (n == 0) return 0;
    while (n-- > 0) {
        if (tolower(*(unsigned char*)s1) != tolower(*(unsigned char*)s2))
            return tolower(*(unsigned char*)s1) - tolower(*(unsigned char*)s2);
        if (*s1 == '\0') return 0;
        s1++; s2++;
    }
    return 0;
}

int strncmp(const char* s1, const char* s2, size_t n) {
    if (n == 0) return 0;
    while (n-- > 0) {
        if (*(unsigned char*)s1 != *(unsigned char*)s2)
            return *(unsigned char*)s1 - *(unsigned char*)s2;
        if (*s1 == '\0') return 0;
        s1++; s2++;
    }
    return 0;
}
char* strstr(const char* h, const char* n) {
    size_t nl = strlen(n);
    if (!nl) return (char*)h;
    while (*h) { if (!strncmp(h, n, nl)) return (char*)h; h++; }
    return NULL;
}
char* strncpy(char* dest, const char* src, size_t n) {
    size_t i;
    for (i = 0; i < n && src[i] != '\0'; i++) dest[i] = src[i];
    for (; i < n; i++) dest[i] = '\0';
    return dest;
}
size_t strlen(const char* s) {
    size_t len = 0;
    while (s[len]) len++;
    return len;
}

char* strrchr(const char* s, int c) {
    char* last = NULL;
    do { if (*s == (char)c) last = (char*)s; } while (*s++);
    return last;
}

char* strchr(const char* s, int c) {
    while (*s) { if (*s == (char)c) return (char*)s; s++; }
    return NULL;
}

void* memset(void* s, int c, size_t n) {
    if (!s) return s;
    unsigned char* p = (unsigned char*)s;
    while (n--) *p++ = (unsigned char)c;
    return s;
}

void* memmove(void* dest, const void* src, size_t n) {
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    if (d < s) {
        while (n--) *d++ = *s++;
    } else {
        d += n; s += n;
        while (n--) *--d = *--s;
    }
    return dest;
}

void* memcpy(void* dest, const void* src, size_t n) {
    if (!dest || !src) return dest;
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    while (n--) *d++ = *s++;
    return dest;
}

int isspace(int c) { return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r'; }
int tolower(int c) { return (c >= 'A' && c <= 'Z') ? (c + 'a' - 'A') : c; }
int toupper(int c) { return (c >= 'a' && c <= 'z') ? (c - 'a' + 'A') : c; }
int isupper(int c) { return (c >= 'A' && c <= 'Z'); }
int atoi(const char* s) {
    int res = 0;
    while (*s >= '0' && *s <= '9') { res = res * 10 + (*s - '0'); s++; }
    return res;
}
long atof(const char* s) { (void)s; return 0; }
int sscanf(const char* s, const char* f, ...) { (void)s; (void)f; return 0; }
int system(const char* c) { (void)c; return -1; }
int abs(int j) { return j < 0 ? -j : j; }
long fabs(long x) { return x < 0 ? -x : x; }
int remove(const char* p) { (void)p; return -1; }
int rename(const char* o, const char* n) { (void)o; (void)n; return -1; }
int mkdir(const char *p, int m) { (void)p; (void)m; return -1; }
static int dummy_errno = 0;
int* __errno_location(void) { return &dummy_errno; }
extern "C" void serial_print(const char* s);

FILE* fopen(const char* p, const char* m) {
    serial_print("FOPEN: ");
    serial_print(p);
    serial_print(" mode: ");
    serial_print(m);
    serial_print("\n");
    
    if (strstr(p, "DOOM1.WAD") || strstr(p, "doom1.wad")) {
        auto res = get_module_response();
        if (!res) {
            serial_print("MOD_RES NULL\n");
            goto dummy;
        }
        
        for (uint64_t i = 0; i < res->module_count; i++) {
            serial_print("MOD: ");
            serial_print(res->modules[i]->path);
            serial_print("\n");
            if (strstr(res->modules[i]->path, "DOOM1.WAD") || strstr(res->modules[i]->path, "doom1.wad")) {
                FILE* f = (FILE*)malloc(sizeof(FILE));
                f->data = (uint8_t*)res->modules[i]->address;
                f->size = res->modules[i]->size;
                f->pos = 0;
                serial_print("WAD FOUND\n");
                return f;
            }
        }
        serial_print("WAD NOT IN MODULES\n");
    }

dummy:

    // Fallback: return a dummy file instead of NULL to prevent crashes
    serial_print("FOPEN DUMMY: ");
    serial_print(p);
    serial_print("\n");
    FILE* f = (FILE*)malloc(sizeof(FILE));
    f->data = NULL;
    f->size = 0;
    f->pos = 0;
    return f;
}
int fclose(FILE* s) { return 0; } // Don't free for now to avoid complexity
size_t fread(void* p, size_t s, size_t n, FILE* f) {
    if (!f || !f->data) return 0;
    size_t total = s * n;
    if (f->pos + total > f->size) total = f->size - f->pos;
    memcpy(p, f->data + f->pos, total);
    
    /* Debug print removed to reduce noise, but let's keep a tiny bit if it fails */
    if (total == 0 && s * n > 0) {
        if (global_term) global_term->print("FREAD EOF\n");
    }

    f->pos += total;
    return total / s;
}
int fseek(FILE* s, long o, int w) {
    if (!s) return -1;
    if (w == SEEK_SET) s->pos = o;
    else if (w == SEEK_CUR) s->pos += o;
    else if (w == SEEK_END) s->pos = s->size + o;
    return 0;
}
long ftell(FILE* s) { return s ? (long)s->pos : -1; }
int feof(FILE* s) { return s ? (s->pos >= s->size) : 1; }
int fprintf(FILE* s, const char* f, ...) { (void)s; (void)f; return 0; }
static void min_vsnprintf(char* str, size_t size, const char* format, va_list ap) {
    size_t i = 0, j = 0;
    while (format[i] && j < size - 1) {
        if (format[i] == '%') {
            i++;
            if (format[i] == 's') {
                const char* s = va_arg(ap, const char*);
                while (*s && j < size - 1) str[j++] = *s++;
            } else if (format[i] == '.' || (format[i] >= '0' && format[i] <= '9')) {
                int width = 0;
                while (format[i] >= '0' && format[i] <= '9') {
                    width = width * 10 + (format[i] - '0');
                    i++;
                }
                if (format[i] == '.') {
                    i++;
                    width = 0;
                    while (format[i] >= '0' && format[i] <= '9') {
                        width = width * 10 + (format[i] - '0');
                        i++;
                    }
                }
                if (format[i] == 'd' || format[i] == 'i') {
                    int d = va_arg(ap, int);
                    char buf[16];
                    itoa(d, buf, 10);
                    int len = strlen(buf);
                    while (len < width && j < size - 1) {
                        str[j++] = '0';
                        len++;
                    }
                    char* p = buf;
                    while (*p && j < size - 1) str[j++] = *p++;
                }
                } else if (format[i] == 'x' || format[i] == 'p') {
                unsigned long d = (format[i] == 'p') ? (unsigned long)va_arg(ap, void*) : va_arg(ap, unsigned int);
                char buf[20];
                itoa(d, buf, 16);
                char* p = buf;
                while (*p && j < size - 1) str[j++] = *p++;
            } else if (format[i] == 'd' || format[i] == 'i') {
                int d = va_arg(ap, int);
                char buf[16];
                itoa(d, buf, 10);
                char* p = buf;
                while (*p && j < size - 1) str[j++] = *p++;
            }
 else {
                str[j++] = '%';
                if (j < size - 1) str[j++] = format[i];
            }
        } else {
            str[j++] = format[i];
        }
        i++;
    }
    str[j] = '\0';
}


int printf(const char* f, ...) {
    char buf[256];
    va_list ap;
    va_start(ap, f);
    min_vsnprintf(buf, sizeof(buf), f, ap);
    va_end(ap);
    if (global_term) global_term->print(buf);
    serial_print(buf);
    return 0;
}
int vfprintf(FILE* s, const char* f, va_list a) {
    (void)s;
    char buf[256];
    min_vsnprintf(buf, sizeof(buf), f, a);
    if (global_term) global_term->print(buf);
    serial_print(buf);
    return 0;
}
int vsnprintf(char* str, size_t size, const char* format, va_list ap) {
    min_vsnprintf(str, size, format, ap);
    return strlen(str);
}
size_t fwrite(const void* p, size_t s, size_t n, FILE* f) { (void)p; (void)s; (void)n; (void)f; return 0; }
int putchar(int c) { 
    char s[2] = {(char)c, 0};
    serial_print(s);
    return 0; 
}
int puts(const char* s) { 
    if (global_term) { global_term->print(s); global_term->print("\n"); } 
    serial_print(s); 
    serial_print("\n");
    return 0; 
}
int snprintf(char* str, size_t size, const char* format, ...) {
    va_list ap;
    va_start(ap, format);
    min_vsnprintf(str, size, format, ap);
    va_end(ap);
    return strlen(str);
}
int fflush(FILE* s) { (void)s; return 0; }
extern bool doom_running;
void exit(int s) { 
    (void)s;
    doom_running = false; 
}

}
