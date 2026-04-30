#pragma once
#include "limine.h"
#include <stdint.h>

extern uint32_t screen_width;
extern uint32_t screen_height;

void draw_rectangle(limine_framebuffer* fb, int x, int y, int width, int height, uint32_t color);
void draw_rectangle_alpha(limine_framebuffer* fb, int x, int y, int width, int height, uint32_t color, uint8_t alpha);
void draw_wallpaper(limine_framebuffer* fb, uint32_t color);
void draw_circle(limine_framebuffer* fb, int x, int y, int radius, uint32_t color);
void create_clickable_button(limine_framebuffer* fb, int x, int y, int width, int height, const char* shape, int radius, void (*callback)(), const char* label);
void set_background(limine_framebuffer* fb, uint32_t hex_code);
void draw_string(limine_framebuffer* fb, int x, int y, const char* s, uint32_t color);
extern "C" void draw_cursor(limine_framebuffer* fb, int x, int y);
