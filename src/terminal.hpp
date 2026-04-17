#pragma once
#include <stdint.h>
#include "sboot.h"
#include "font.hpp"

extern "C" uint32_t get_timer_ticks();

class Terminal;
void execute_command(const char* buffer, Terminal* term);

class Terminal {
public:
    sboot_framebuffer* fb;
    int win_x, win_y, win_width, win_height;
    char command_buffer[128];
    int cmd_len;
    bool is_open;

    uint32_t* current_buffer;
    // Massive padding to prevent any overflow issues
    char text_screen[80 * 50]; 
    uint32_t color_screen[80 * 50];
    int char_x, char_y;

    void set_window(int x, int y, int w, int h) {
        win_x = x; win_y = y; win_width = w; win_height = h;
        char_x = 0; char_y = 0;
        for(int i=0; i < 80*50; i++) {
            text_screen[i] = ' ';
            color_screen[i] = 0xFFFFFF;
        }
    }

    void draw_char_at(char c, int cx, int cy, uint32_t color) {
        uint8_t bitmap[8];
        if (!get_char_bitmap(c, bitmap)) return;
        int stride = fb->pitch / 4;
        int px_start = win_x + 8 + cx * 8;
        int py_start = win_y + 35 + cy * 10;

        for (int row = 0; row < 8; row++) {
            if (py_start + row >= (int)fb->height) continue;
            for (int col = 0; col < 8; col++) {
                if (px_start + col >= (int)fb->width) continue;
                if (bitmap[row] & (1 << col)) {
                    int py = py_start + row;
                    int px = px_start + col;
                    // Strict Clipping
                    if (py >= win_y + 30 && py < win_y + win_height - 2 && px >= win_x + 2 && px < win_x + win_width - 2)
                        current_buffer[py * stride + px] = color;
                }
            }
        }
    }

    void render() {
        if (!is_open) return;
        for (int y = 0; y < 25; y++) {
            for (int x = 0; x < 70; x++) {
                char c = text_screen[y * 80 + x];
                if (c != ' ') draw_char_at(c, x, y, color_screen[y * 80 + x]);
            }
        }
        if ((get_timer_ticks() / 500) % 2 == 0) {
            draw_char_at('_', char_x, char_y, 0xFFFFFF);
        }
    }

    void newline() {
        char_x = 0;
        char_y++;
        if (char_y >= 25) {
            for (int y = 0; y < 24; y++) {
                for (int x = 0; x < 80; x++) {
                    text_screen[y * 80 + x] = text_screen[(y + 1) * 80 + x];
                    color_screen[y * 80 + x] = color_screen[(y + 1) * 80 + x];
                }
            }
            for (int x = 0; x < 80; x++) {
                text_screen[24 * 80 + x] = ' ';
                color_screen[24 * 80 + x] = 0xFFFFFF;
            }
            char_y = 24;
        }
    }

    Terminal(sboot_framebuffer* framebuffer) : fb(framebuffer), cmd_len(0), is_open(false), current_buffer(nullptr) {
        set_window(0, 0, (int)fb->width, (int)fb->height);
    }

    void print(const char* s, uint32_t color = 0xFFFFFF) {
        while (*s) {
            char c = *s++;
            if (c == '\n') newline();
            else {
                if (char_x < 70) {
                    text_screen[char_y * 80 + char_x] = c;
                    color_screen[char_y * 80 + char_x] = color;
                    char_x++;
                } else {
                    newline();
                    if (char_y < 50) { // Strict overflow protection
                        text_screen[char_y * 80 + char_x] = c;
                        color_screen[char_y * 80 + char_x] = color;
                        char_x++;
                    }
                }
            }
        }
    }

    void print_char(char c) {
        if (c == '\n') {
            command_buffer[cmd_len] = '\0';
            print("\n");
            execute_command(command_buffer, this);
            cmd_len = 0;
            print_prompt();
        } else if (c == 8) {
            if (cmd_len > 0) {
                if (char_x > 0) char_x--;
                text_screen[char_y * 80 + char_x] = ' ';
                cmd_len--;
            }
        } else {
            if (char_x < 65) {
                text_screen[char_y * 80 + char_x] = c;
                color_screen[char_y * 80 + char_x] = 0xFFFFFF;
                char_x++;
                if (cmd_len < 127) command_buffer[cmd_len++] = c;
            }
        }
    }

    void print_prompt() {
        print("admin@Gridz-OS:~$ ", 0x00FF00);
    }
};

#include "commands.hpp"
