#ifndef DOOM_GRIDZ_LIBC_H
#define DOOM_GRIDZ_LIBC_H

#include <stddef.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

void* malloc(size_t size);
void* calloc(size_t nmemb, size_t size);
void free(void* ptr);
void* realloc(void* ptr, size_t size);
void* memset(void* s, int c, size_t n);
void* memcpy(void* dest, const void* src, size_t n);
void* memmove(void* dest, const void* src, size_t n);
size_t strlen(const char* s);
char* strdup(const char* s);
int strcmp(const char* s1, const char* s2);
int strncmp(const char* s1, const char* s2, size_t n);
char* strstr(const char* haystack, const char* needle);
char* strncpy(char* dest, const char* src, size_t n);
int strcasecmp(const char* s1, const char* s2);
int strncasecmp(const char* s1, const char* s2, size_t n);
char* strrchr(const char* s, int c);
char* strchr(const char* s, int c);

typedef struct FILE FILE;
#define stderr ((FILE*)2)
#define stdout ((FILE*)1)

FILE* fopen(const char* path, const char* mode);
int fclose(FILE* stream);
size_t fread(void* ptr, size_t size, size_t nmemb, FILE* stream);
int fseek(FILE* stream, long offset, int whence);
long ftell(FILE* stream);
int feof(FILE* stream);
int fprintf(FILE* stream, const char* format, ...);
int printf(const char* format, ...);
int vfprintf(FILE* stream, const char* format, va_list ap);
int vsnprintf(char* str, size_t size, const char* format, va_list ap);
size_t fwrite(const void* ptr, size_t size, size_t nmemb, FILE* stream);
int putchar(int c);
int puts(const char* s);
int snprintf(char* str, size_t size, const char* format, ...);
int fflush(FILE* stream);
int isspace(int c);
int isupper(int c);
int isalpha(int c);
int isdigit(int c);
int tolower(int c);
int toupper(int c);
int atoi(const char* nptr);
long atof(const char* nptr);
int sscanf(const char* str, const char* format, ...);
int system(const char* command);
int abs(int j);
long fabs(long x);
int remove(const char* pathname);
int rename(const char* oldpath, const char* newpath);
int mkdir(const char *pathname, int mode);
int* __errno_location(void);
void exit(int status);

#define DIR_SEPARATOR '/'

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#define INT_MAX 2147483647
#define INT_MIN (-INT_MAX - 1)
#define SHRT_MAX 32767
#define DBL_MAX 1e37

#ifdef __cplusplus
}
#endif

#endif
