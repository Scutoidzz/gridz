#pragma once

#include <stddef.h>
#include <stdint.h>

/**
 * Gridz OS Custom String Utility Header
 * Provides standard memory and string functions for freestanding environments.
 */

extern "C" {

// --- Memory Functions ---

inline void* memcpy(void* dest, const void* src, size_t n) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    while (n--) {
        *d++ = *s++;
    }
    return dest;
}

inline void* memset(void* s, int c, size_t n) {
    uint8_t* p = (uint8_t*)s;
    while (n--) {
        *p++ = (uint8_t)c;
    }
    return s;
}

inline void* memmove(void* dest, const void* src, size_t n) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    if (d < s) {
        while (n--) {
            *d++ = *s++;
        }
    } else {
        d += n;
        s += n;
        while (n--) {
            *--d = *--s;
        }
    }
    return dest;
}

inline int memcmp(const void* s1, const void* s2, size_t n) {
    const uint8_t* p1 = (const uint8_t*)s1;
    const uint8_t* p2 = (const uint8_t*)s2;
    while (n--) {
        if (*p1 != *p2) {
            return *p1 - *p2;
        }
        p1++;
        p2++;
    }
    return 0;
}

// --- String Functions ---

inline size_t strlen(const char* s) {
    size_t len = 0;
    while (s[len]) {
        len++;
    }
    return len;
}

inline char* strcpy(char* dest, const char* src) {
    char* d = dest;
    while ((*d++ = *src++))
        ;
    return dest;
}

inline char* strncpy(char* dest, const char* src, size_t n) {
    char* d = dest;
    while (n && (*d++ = *src++)) {
        n--;
    }
    while (n--) {
        *d++ = '\0';
    }
    return dest;
}

inline int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

inline int strncmp(const char* s1, const char* s2, size_t n) {
    while (n && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0) return 0;
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

inline char* strcat(char* dest, const char* src) {
    char* d = dest;
    while (*d) d++;
    while ((*d++ = *src++))
        ;
    return dest;
}

inline char* strchr(const char* s, int c) {
    while (*s != (char)c) {
        if (!*s) return nullptr;
        s++;
    }
    return (char*)s;
}

inline char* strstr(const char* haystack, const char* needle) {
    size_t n = strlen(needle);
    if (n == 0) return (char*)haystack;
    while (*haystack) {
        if (!memcmp(haystack, needle, n)) {
            return (char*)haystack;
        }
        haystack++;
    }
    return nullptr;
}


} // extern "C"

// --- String Class ---

extern "C" void* malloc(size_t size);
extern "C" void free(void* ptr);

class String {
private:
    char* _data;
    size_t _len;

public:
    String() : _data(nullptr), _len(0) {}

    String(const char* s) {
        if (s) {
            _len = strlen(s);
            _data = (char*)malloc(_len + 1);
            if (_data) {
                memcpy(_data, s, _len + 1);
            }
        } else {
            _data = nullptr;
            _len = 0;
        }
    }

    // Copy constructor
    String(const String& other) {
        _len = other._len;
        if (other._data) {
            _data = (char*)malloc(_len + 1);
            if (_data) {
                memcpy(_data, other._data, _len + 1);
            }
        } else {
            _data = nullptr;
        }
    }

    ~String() {
        if (_data) {
            free(_data);
        }
    }

    const char* c_str() const { return _data ? _data : ""; }
    size_t length() const { return _len; }

    bool operator==(const char* s) const {
        return strcmp(c_str(), s) == 0;
    }

    bool operator==(const String& other) const {
        if (_len != other._len) return false;
        return strcmp(c_str(), other.c_str()) == 0;
    }

    String& operator=(const char* s) {
        if (_data) free(_data);
        if (s) {
            _len = strlen(s);
            _data = (char*)malloc(_len + 1);
            if (_data) memcpy(_data, s, _len + 1);
        } else {
            _data = nullptr;
            _len = 0;
        }
        return *this;
    }

    String& operator=(const String& other) {
        if (this == &other) return *this;
        if (_data) free(_data);
        _len = other._len;
        if (other._data) {
            _data = (char*)malloc(_len + 1);
            if (_data) memcpy(_data, other._data, _len + 1);
        } else {
            _data = nullptr;
        }
        return *this;
    }
};

using string = String;
