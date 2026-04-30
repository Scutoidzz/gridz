#include "libc_gridz.h"
#include <stdint.h>
#include "terminal.hpp"
#include "limine.h"

extern Terminal* global_term;
extern "C" void serial_print(const char* s);
extern "C" void itoa(uint64_t n, char* s, int base);

extern "C" {

struct limine_module_response* get_module_response();

struct FILE {
    uint8_t* data;
    size_t size;
    size_t pos;
};




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

static void str_append(char* dst, int* pos, const char* src, int max) {
    while (*src && *pos < max) dst[(*pos)++] = *src++;
    dst[*pos] = '\0';
}

FILE* fopen(const char* p, const char* m) {
    serial_print("FOPEN: ");
    serial_print(p);
    serial_print(" mode: ");
    serial_print(m);
    serial_print("\n");
    
    // Try to find ANY .WAD file if requested
    bool is_wad = strstr(p, ".WAD") || strstr(p, ".wad");
    
    if (is_wad) {
        auto res = get_module_response();
        if (!res) {
            if (global_term) global_term->print("ERROR: Module response NULL\n");
            return NULL;
        }
        
        for (uint64_t i = 0; i < res->module_count; i++) {
            // Check if module path or cmdline contains the filename (case insensitive-ish)
            const char* m_path = res->modules[i]->path;
            const char* m_cmd  = res->modules[i]->cmdline;
            
            // Basic case-insensitive check for common WAD names
            bool match = false;
            if (strstr(m_path, "DOOM") || strstr(m_path, "doom") ||
                strstr(m_cmd, "DOOM") || strstr(m_cmd, "doom")) {
                match = true;
            }
                         
            if (match) {
                FILE* f = (FILE*)malloc(sizeof(FILE));
                f->data = (uint8_t*)res->modules[i]->address;
                f->size = res->modules[i]->size;
                f->pos = 0;
                
                if (global_term) {
                    global_term->print("Loaded module: ");
                    global_term->print(m_path);
                    global_term->print("\nSize: ");
                    char sb[20]; itoa(f->size, sb, 10);
                    global_term->print(sb); global_term->print(" bytes\n");
                }
                return f;
            }
        }
        if (global_term) {
            global_term->print("ERROR: WAD not found in modules. Paths seen:\n");
            for (uint64_t i = 0; i < res->module_count; i++) {
                global_term->print(" - ");
                global_term->print(res->modules[i]->path);
                global_term->print("\n");
            }
        }
    }

    return NULL;
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
