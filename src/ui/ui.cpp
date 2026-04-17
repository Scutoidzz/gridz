#include "ui.hpp"
#include "../font.hpp"

void draw_rect(sboot_framebuffer* fb, uint32_t* buffer, int x, int y, int width, int height, uint32_t color) {
    int stride = fb->pitch / 4;
    for (int row = 0; row < height; row++) {
        int py = y + row;
        if (py < 0 || py >= (int)fb->height) continue;
        for (int col = 0; col < width; col++) {
            int px = x + col;
            if (px < 0 || px >= (int)fb->width) continue;
            buffer[py * stride + px] = color;
        }
    }
}

void draw_button(sboot_framebuffer* fb, uint32_t* buffer, Button& btn) {
    draw_rect(fb, buffer, btn.x, btn.y, btn.width, btn.height, btn.color);
    for (int i = 0; btn.label[i] != '\0'; i++) {
        uint8_t bitmap[8];
        if (get_char_bitmap(btn.label[i], bitmap)) {
            int stride = fb->pitch / 4;
            for (int row = 0; row < 8; row++) {
                for (int col = 0; col < 8; col++) {
                    if (bitmap[row] & (1 << col)) {
                        int py = btn.y + btn.height/2 - 4 + row;
                        int px = btn.x + 10 + i * 8 + col;
                        if (py >= 0 && py < (int)fb->height && px >= 0 && px < (int)fb->width)
                            buffer[py * stride + px] = 0xFFFFFF;
                    }
                }
            }
        }
    }
}

void draw_cursor(sboot_framebuffer* fb, uint32_t* buffer, int x, int y) {
    int radius = 6;
    for (int i = -radius; i <= radius; i++) {
        for (int j = -radius; j <= radius; j++) {
            int dist_sq = i*i + j*j;
            if (dist_sq <= radius*radius) {
                uint32_t color = (dist_sq >= (radius-2)*(radius-2)) ? 0xFFFFFF : 0x007ACC;
                draw_rect(fb, buffer, x + j, y + i, 1, 1, color);
            }
        }
    }
}

void draw_launcher(sboot_framebuffer* fb, uint32_t* buffer, bool terminal_open) {
    // Taskbar
    draw_rect(fb, buffer, 0, fb->height - 40, fb->width, 40, 0x1A1A1A);
    // Terminal Button
    Button term_btn = {10, (int)fb->height - 35, 100, 30, "Terminal", 0x333333, nullptr};
    draw_button(fb, buffer, term_btn);
    if (terminal_open) {
        draw_rect(fb, buffer, 50, 50, 600, 300, 0x111111);
        draw_rect(fb, buffer, 50, 50, 600, 25, 0x007ACC);
        draw_rect(fb, buffer, 52, 75, 596, 273, 0x1E1E1E);
    }
}