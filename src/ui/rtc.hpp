#pragma once
#include <stdint.h>
#include "../io.hpp"

static inline uint8_t rtc_read(uint8_t reg) {
    outb(0x70, (uint8_t)(0x80 | reg));
    return inb(0x71);
}

static inline uint8_t bcd2bin(uint8_t v) {
    return (uint8_t)((v & 0x0F) + (v >> 4) * 10);
}

struct RTCTime { uint8_t h, m, s; };

static inline RTCTime rtc_now() {
    // Spin while update-in-progress flag is set
    while (rtc_read(0x0A) & 0x80) {}
    uint8_t s  = rtc_read(0x00);
    uint8_t mn = rtc_read(0x02);
    uint8_t h  = rtc_read(0x04);
    uint8_t regB = rtc_read(0x0B);
    if (!(regB & 0x04)) { s = bcd2bin(s); mn = bcd2bin(mn); h = bcd2bin(h); }
    // 12-hour → 24-hour
    if (!(regB & 0x02) && (h & 0x80)) { h = (uint8_t)(((h & 0x7F) + 12) % 24); }
    RTCTime t; t.h = h; t.m = mn; t.s = s;
    return t;
}

// Format: "HH:MM:SS" into buf[9]
static inline void rtc_format(RTCTime t, char* buf) {
    auto d2 = [](char* p, uint8_t v) {
        p[0] = '0' + v/10; p[1] = '0' + v%10;
    };
    d2(buf+0, t.h); buf[2] = ':';
    d2(buf+3, t.m); buf[5] = ':';
    d2(buf+6, t.s); buf[8] = '\0';
}
