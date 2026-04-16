#pragma once
#include <stdint.h>
#include "limine.h"
#include "font.hpp"
class Terminal;
void execute_command(const char* buffer, Terminal* term);

class Terminal {
public:
    limine_framebuffer* fb;
    int cursor_x;
    int cursor_y;
    char command_buffer[64];
    int cmd_len;

    void draw_char(char c, int x, int y, uint32_t color) {
        uint8_t bitmap[8];
        if (!get_char_bitmap(c, bitmap)) return;

        uint32_t *fb_ptr = (uint32_t *)fb->address;
        int stride = fb->pitch / 4;

        for (int row = 0; row < 8; row++) {
            for (int col = 0; col < 8; col++) {
                if (bitmap[row] & (1 << col)) {
                    fb_ptr[(y + row) * stride + (x + col)] = color;
                } else {
                    fb_ptr[(y + row) * stride + (x + col)] = 0x000000;
                }
            }
        }
    }

    void newline() {
        cursor_x = 0;
        cursor_y += 8;
        if (cursor_y >= (int)fb->height) {
            uint32_t *fb_ptr = (uint32_t *)fb->address;
            int stride = fb->pitch / 4;
            for (int y = 0; y < (int)fb->height - 8; y++) {
                for (int x = 0; x < (int)fb->width; x++) {
                    fb_ptr[y * stride + x] = fb_ptr[(y + 8) * stride + x];
                }
            }
            for (int y = (int)fb->height - 8; y < (int)fb->height; y++) {
                for (int x = 0; x < (int)fb->width; x++) {
                    fb_ptr[y * stride + x] = 0x000000;
                }
            }
            cursor_y -= 8;
        }
    }

public:
    Terminal(limine_framebuffer* framebuffer) : fb(framebuffer), cursor_x(0), cursor_y(0), cmd_len(0) {}

    void print_char(char c) {
        if (c == '\n') {
            command_buffer[cmd_len] = '\0';
            execute_command(command_buffer, this);
            cmd_len = 0;
            newline();
        } else if (c == 8) {
            if (cmd_len > 0) { 
                if (cursor_x >= 8) {
                    cursor_x -= 8;
                } else if (cursor_y >= 8) {
                    cursor_y -= 8;
                    cursor_x = fb->width - (fb->width % 8) - 8;
                }
                draw_char(' ', cursor_x, cursor_y, 0x000000);
                cmd_len--;
            }
        } else {
            draw_char(c, cursor_x, cursor_y, 0xFFFFFF);
            if (cmd_len < 63) {
                command_buffer[cmd_len] = c;
                cmd_len++;
            }
            cursor_x += 8;
            
            if (cursor_x >= (int)fb->width) { 
                newline();
            }
        }
    }

    void print_prompt() {
        draw_char('>', cursor_x, cursor_y, 0xFFFFFF);
        cursor_x += 8;
    }
};

#include "commands.hpp"
