#include "libc_gridz.h"
#include <stdint.h>
#include "../../terminal.hpp"
#include "../limine.h"

extern Terminal* global_term;

extern "C" {

struct limine_module_response* get_module_response();

struct FILE {
    uint8_t* data;
    size_t size;
    size_t pos;
};

void itoa(int n, char* s, int base) {
    static const char digits[] = "0123456789ABCDEF";
    char* p = s;
    if (n == 0) { *p++ = '0'; *p = '\0'; return; }
    bool neg = false;
    if (n < 0 && base == 10) { neg = true; n = -n; }
    unsigned int un = (unsigned int)n;
    while (un > 0) { *p++ = digits[un % base]; un /= base; }
    if (neg) *p++ = '-';
    *p = '\0';
    char* q = s; p--;
    while (q < p) { char t = *q; *q = *p; *p = t; q++; p--; }
}

#define HEAP_SIZE (1024 * 1024 * 32)
static uint8_t heap[HEAP_SIZE];
static size_t heap_ptr = 0;

void* malloc(size_t size) {
    if (heap_ptr + size > HEAP_SIZE) return NULL;
    void* ptr = &heap[heap_ptr];
    heap_ptr += (size + 15) & ~15;
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
    void* new_ptr = malloc(size);
    if (ptr) memcpy(new_ptr, ptr, size); // Oversimplified but might work
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
    if (global_term) {
        // Only print to serial for now to keep screen clean
        // global_term->print("FOPEN: ");
        // global_term->print(p);
        // global_term->print("\n");
    }
    serial_print("FOPEN: ");
    serial_print(p);
    serial_print(" mode: ");
    serial_print(m);
    serial_print("\n");
    
    // Check if it's our WAD file
    bool is_our_wad = false;
    if (strstr(p, "DOOM1.WAD") || strstr(p, "doom1.wad")) is_our_wad = true;
    
    if (!is_our_wad) return NULL;

    auto res = get_module_response();
    if (!res || res->module_count == 0) return NULL;
    
    uint8_t* data = (uint8_t*)res->modules[0]->address;
    if (global_term) {
        global_term->print("WAD Magic: ");
        char m[5] = { (char)data[0], (char)data[1], (char)data[2], (char)data[3], 0 };
        global_term->print(m);
        global_term->print("\n");
    }

    FILE* f = (FILE*)malloc(sizeof(FILE));
    f->data = (uint8_t*)res->modules[0]->address;
    f->size = res->modules[0]->size;
    f->pos = 0;
    return f;
}
int fclose(FILE* s) { if (s) free(s); return 0; }
size_t fread(void* p, size_t s, size_t n, FILE* f) {
    if (!f) return 0;
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
            } else if (format[i] == 'd' || format[i] == 'i') {
                int d = va_arg(ap, int);
                char buf[16];
                itoa(d, buf, 10);
                char* p = buf;
                while (*p && j < size - 1) str[j++] = *p++;
            } else {
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
void exit(int s) { if (global_term) global_term->print("DOOM EXIT CALLED\n"); while(1); }

}
