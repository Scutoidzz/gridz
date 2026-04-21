#include "comp.hpp"
#include "../../font.hpp"
#include "../../font8x8_basic.h"

uint32_t screen_width = 0;
uint32_t screen_height = 0;

void draw_rectangle(limine_framebuffer* fb, int x, int y, int width, int height, uint32_t color) {
    if (!fb) return;
    
    screen_width = fb->width;
    screen_height = fb->height;

    // Bounds checking
    if (x < 0) { width += x; x = 0; }
    if (y < 0) { height += y; y = 0; }
    if (x >= (int)screen_width || y >= (int)screen_height) return;
    if (x + width > (int)screen_width) width = screen_width - x;
    if (y + height > (int)screen_height) height = screen_height - y;
    if (width <= 0 || height <= 0) return;

    uint32_t *fb_ptr = (uint32_t *)fb->address;
    int stride = fb->pitch / 4;

    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            fb_ptr[(y + row) * stride + (x + col)] = color;
        }
    }
}

void draw_rectangle_alpha(limine_framebuffer* fb, int x, int y, int width, int height, uint32_t color, uint8_t alpha) {
    if (!fb) return;
    
    screen_width = fb->width;
    screen_height = fb->height;

    // Bounds checking
    if (x < 0) { width += x; x = 0; }
    if (y < 0) { height += y; y = 0; }
    if (x >= (int)screen_width || y >= (int)screen_height) return;
    if (x + width > (int)screen_width) width = screen_width - x;
    if (y + height > (int)screen_height) height = screen_height - y;
    if (width <= 0 || height <= 0) return;

    uint32_t *fb_ptr = (uint32_t *)fb->address;
    int stride = fb->pitch / 4;

    uint8_t r_src = (color >> 16) & 0xFF;
    uint8_t g_src = (color >> 8) & 0xFF;
    uint8_t b_src = color & 0xFF;

    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            uint32_t dest = fb_ptr[(y + row) * stride + (x + col)];
            
            uint8_t r_dest = (dest >> 16) & 0xFF;
            uint8_t g_dest = (dest >> 8) & 0xFF;
            uint8_t b_dest = dest & 0xFF;

            uint8_t r = (alpha * r_src + (255 - alpha) * r_dest) / 255;
            uint8_t g = (alpha * g_src + (255 - alpha) * g_dest) / 255;
            uint8_t b = (alpha * b_src + (255 - alpha) * b_dest) / 255;

            fb_ptr[(y + row) * stride + (x + col)] = (r << 16) | (g << 8) | b;
        }
    }
}

void draw_wallpaper(limine_framebuffer* fb, uint32_t color) {
    if (!fb) return;
    draw_rectangle(fb, 0, 0, fb->width, fb->height, color);
}

void draw_circle(limine_framebuffer* fb, int x, int y, int radius, uint32_t color) {
    if (!fb) return;
    
    screen_width = fb->width;
    screen_height = fb->height;

    uint32_t *fb_ptr = (uint32_t *)fb->address;
    int stride = fb->pitch / 4;

    for (int row = -radius; row <= radius; row++) {
        for (int col = -radius; col <= radius; col++) {
            if (col * col + row * row <= radius * radius) {
                int px = x + col;
                int py = y + row;
                
                // Bounds checking
                if (px >= 0 && px < (int)screen_width && py >= 0 && py < (int)screen_height) {
                    fb_ptr[py * stride + px] = color;
                }
            }
        }
    }
}

static bool strings_equal(const char* s1, const char* s2) {
    if (!s1 || !s2) return s1 == s2;
    while (*s1 && *s2) {
        if (*s1 != *s2) return false;
        s1++;
        s2++;
    }
    return *s1 == *s2;
}

void draw_string(limine_framebuffer* fb, int x, int y, const char* s, uint32_t color) {
    if (!fb || !s) return;
    int cur_x = x;
    while (*s) {
        char c = *s++;
        uint8_t bitmap[8];
        if (get_char_bitmap(c, bitmap)) {
            uint32_t *fb_ptr = (uint32_t *)fb->address;
            int stride = fb->pitch / 4;
            for (int row = 0; row < 8; row++) {
                for (int col = 0; col < 8; col++) {
                    if (bitmap[row] & (1 << col)) {
                        int px = cur_x + col;
                        int py = y + row;
                        if (px >= 0 && px < (int)fb->width && py >= 0 && py < (int)fb->height) {
                            fb_ptr[py * stride + px] = color;
                        }
                    }
                }
            }
        }
        cur_x += 8;
    }
}

void create_clickable_button(limine_framebuffer* fb, int x, int y, int width, int height, const char* shape, int radius, void (*callback)(), const char* label) {
    if (!fb) return;

    if (strings_equal(shape, "circle")) {
        draw_circle(fb, x, y, radius, 0x00FF00); // Green circle button
    } else {
        draw_rectangle(fb, x, y, width, height, 0x0000FF); // Blue rectangle button
    }
    
    if (label) {
        int text_width = 0;
        const char* p = label;
        while (*p++) text_width += 8;
        
        int text_x, text_y;
        if (strings_equal(shape, "circle")) {
            text_x = x - text_width / 2;
            text_y = y - 4;
        } else {
            text_x = x + (width - text_width) / 2;
            text_y = y + (height - 8) / 2;
        }
        
        draw_string(fb, text_x, text_y, label, 0xFFFFFF);
    }
    
    (void)callback;
}

void set_background(limine_framebuffer* fb, uint32_t hex_code) {
    if (!fb) return;
    draw_wallpaper(fb, hex_code);
}

// ── Software arrow cursor ─────────────────────────────────────────────────────
// 12 wide × 19 tall.  Each row is a bitmask: bit11=leftmost pixel.
// '1' = white fill, '2' = black border (drawn first so fill sits on top).
static const uint16_t cursor_fill[19] = {
    0b100000000000,
    0b110000000000,
    0b111000000000,
    0b111100000000,
    0b111110000000,
    0b111111000000,
    0b111111100000,
    0b111111110000,
    0b111111111000,
    0b111111111100,
    0b111111110000,
    0b111101110000,
    0b110001110000,
    0b000001110000,
    0b000001110000,
    0b000000110000,
    0b000000110000,
    0b000000000000,
    0b000000000000,
};
static const uint16_t cursor_border[19] = {
    0b110000000000,
    0b111000000000,
    0b111100000000,
    0b111110000000,
    0b111111000000,
    0b111111100000,
    0b111111110000,
    0b111111111000,
    0b111111111100,
    0b111111111110,
    0b111111111000,
    0b111111111000,
    0b111001111000,
    0b100001111000,
    0b000001111000,
    0b000001111000,
    0b000000111000,
    0b000000110000,
    0b000000000000,
};

extern "C" void draw_cursor(limine_framebuffer* fb, int cx, int cy) {
    if (!fb) return;
    uint32_t *fb_ptr = (uint32_t *)fb->address;
    int stride = fb->pitch / 4;
    int W = (int)fb->width;
    int H = (int)fb->height;

    for (int row = 0; row < 19; row++) {
        for (int col = 0; col < 12; col++) {
            int px = cx + col;
            int py = cy + row;
            if (px < 0 || px >= W || py < 0 || py >= H) continue;

            uint16_t bit = (uint16_t)(0x800 >> col);
            if (cursor_border[row] & bit) {
                fb_ptr[py * stride + px] = 0x000000; // border
            }
        }
    }
    for (int row = 0; row < 19; row++) {
        for (int col = 0; col < 12; col++) {
            int px = cx + col;
            int py = cy + row;
            if (px < 0 || px >= W || py < 0 || py >= H) continue;

            uint16_t bit = (uint16_t)(0x800 >> col);
            if (cursor_fill[row] & bit) {
                fb_ptr[py * stride + px] = 0xFFFFFF; // white fill
            }
        }
    }
}