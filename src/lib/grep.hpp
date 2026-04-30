#pragma once
#include <stddef.h>

#include "string.hpp"

extern "C" void* malloc(size_t size);

#include "fs/fat32.hpp"

// --- Buffer-based core functions (the "generic input" versions) ---

inline bool grep_true(const char* buf, size_t size, const char* pattern) {
    size_t pattern_len = strlen(pattern);
    if (pattern_len == 0) return false;
    for (size_t i = 0; i + pattern_len <= size; i++) {
        bool match = true;
        for (size_t k = 0; k < pattern_len; k++) {
            if (buf[i + k] != pattern[k]) { match = false; break; }
        }
        if (match) return true;
    }
    return false;
}

inline char* grep_text(const char* buf, size_t size, const char* pattern) {
    size_t pattern_len = strlen(pattern);
    if (pattern_len == 0) return nullptr;
    for (size_t i = 0; i + pattern_len <= size; i++) {
        bool match = true;
        for (size_t k = 0; k < pattern_len; k++) {
            if (buf[i + k] != pattern[k]) { match = false; break; }
        }
        if (match) {
            size_t extra = (i + pattern_len + 3 <= size) ? 3 : (size - (i + pattern_len));
            char* res = (char*)malloc(pattern_len + extra + 4);
            if (!res) return nullptr;
            memcpy(res, buf + i, pattern_len + extra);
            memcpy(res + pattern_len + extra, "...", 3);
            res[pattern_len + extra + 3] = '\0';
            return res;
        }
    }
    return nullptr;
}

inline char* search_file(const char* buf, size_t size, const char* pattern, const char* label) {
    size_t pattern_len = strlen(pattern);
    if (pattern_len == 0) return nullptr;

    size_t res_capacity = 8192;
    char* res = (char*)malloc(res_capacity);
    if (!res) return nullptr;
    res[0] = '\0';
    size_t res_len = 0;

    size_t line_start = 0;
    for (size_t i = 0; i <= size; i++) {
        if (i == size || buf[i] == '\n') {
            size_t line_end = i;
            bool found = false;
            for (size_t j = line_start; j + pattern_len <= line_end; j++) {
                bool match = true;
                for (size_t k = 0; k < pattern_len; k++) {
                    if (buf[j + k] != pattern[k]) { match = false; break; }
                }
                if (match) { found = true; break; }
            }
            if (found) {
                size_t name_len = strlen(label);
                size_t line_len = line_end - line_start;
                size_t needed = name_len + 4 + line_len + 1;
                if (res_len + needed < res_capacity - 1) {
                    res[res_len++] = '[';
                    memcpy(res + res_len, label, name_len);
                    res_len += name_len;
                    res[res_len++] = ']';
                    res[res_len++] = ' ';
                    memcpy(res + res_len, buf + line_start, line_len);
                    res_len += line_len;
                    res[res_len++] = '\n';
                    res[res_len] = '\0';
                }
            }
            line_start = i + 1;
        }
    }
    return res;
}

// --- File-based wrappers ---

inline bool grep_true(const char* filename, const char* pattern) {
    size_t size;
    void* data = fat32::read_file(filename, &size);
    if (!data) return false;
    return grep_true((const char*)data, size, pattern);
}

inline char* grep_text(const char* filename, const char* pattern) {
    size_t size;
    void* data = fat32::read_file(filename, &size);
    if (!data) return nullptr;
    return grep_text((const char*)data, size, pattern);
}

inline char* grep_file(const char* filename, const char* pattern) {
    if (grep_true(filename, pattern)) {
        size_t len = strlen(filename);
        char* res = (char*)malloc(len + 1);
        if (!res) return nullptr;
        memcpy(res, filename, len + 1);
        return res;
    }
    return nullptr;
}

inline char* search_file(const char* filename, const char* pattern) {
    size_t size;
    void* data = fat32::read_file(filename, &size);
    if (!data) return nullptr;
    return search_file((const char*)data, size, pattern, filename);
}
