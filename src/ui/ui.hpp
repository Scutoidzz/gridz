#pragma once
#include "../sboot.h"

struct Button {
    int x, y, width, height;
    const char* label;
    uint32_t color;
    void (*callback)();
};

void draw_rect(sboot_framebuffer* fb, uint32_t* buffer, int x, int y, int width, int height, uint32_t color);
void draw_launcher(sboot_framebuffer* fb, uint32_t* buffer, bool terminal_open);
void draw_cursor(sboot_framebuffer* fb, uint32_t* buffer, int x, int y);
void draw_button(sboot_framebuffer* fb, uint32_t* buffer, Button& btn);
