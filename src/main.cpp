#include <stdint.h>
#include <stddef.h>
#include "limine.h"
#include "font.hpp"

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ( "inb %w1, %b0" : "=a"(ret) : "Nd"(port) : "memory");
    return ret;
}

const char kbd_map[128] = {
    0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 8,
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' '
};

void draw_char(struct limine_framebuffer *fb, char c, int x, int y, uint32_t color) {
    uint8_t bitmap[8];
    if (!get_char_bitmap(c, bitmap)) return;
    uint32_t *fb_ptr = (uint32_t *)fb->address;
    int pitch = fb->pitch / 4; // pitch is bytes, pixels are 4 bytes (32bpp)
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            if (bitmap[row] & (1 << col)) {
                fb_ptr[(y + row) * pitch + (x + col)] = color;
            } else {
                fb_ptr[(y + row) * pitch + (x + col)] = 0x000000;
            }
        }
    }
}
extern "C" {
    LIMINE_BASE_REVISION(2)
}

static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0
};

extern "C" void _start(void) {
    
    if (framebuffer_request.response == NULL || framebuffer_request.response->framebuffer_count < 1) {
        while (1) { __asm__("hlt"); }
    }

    struct limine_framebuffer *framebuffer = framebuffer_request.response->framebuffers[0];

    int cursor_x = 0;
    int cursor_y = 0;

    draw_char(framebuffer, '>', cursor_x, cursor_y, 0xFFFFFF);
    cursor_x += 8;

    while (1) {
        if (inb(0x64) & 1) { // checking keyboard status port
            uint8_t scancode = inb(0x60); // read data from keyboard
            if (scancode < 0x80 && kbd_map[scancode]) { // key pressed down
                char c = kbd_map[scancode];
                
                if (c == '\n') {
                    cursor_y += 8;
                    cursor_x = 0;
                    draw_char(framebuffer, '>', cursor_x, cursor_y, 0xFFFFFF);
                    cursor_x += 8;
                } else if (c == 8) { // Backspace
                    if (cursor_x > 8) { // Do not delete the prompt
                        cursor_x -= 8;
                        draw_char(framebuffer, ' ', cursor_x, cursor_y, 0xFFFFFF);
                    }
                } else {
                    draw_char(framebuffer, c, cursor_x, cursor_y, 0xFFFFFF);
                    cursor_x += 8;
                    
                    // Extremely simple text wrapping
                    if (cursor_x >= framebuffer->width) { 
                        cursor_x = 0;
                        cursor_y += 8;
                    }
                }
            }
        }
    }
}