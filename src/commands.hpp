#pragma once
#include "ui/ui.hpp"

inline bool match_cmd(const char* buf, const char* target, int target_len) {
    for (int i = 0; i < target_len; i++) {
        if (buf[i] != target[i]) return false;
    }
    return buf[target_len] == '\0';
}

#include "doom/doomgeneric/doomgeneric.h"

inline void execute_command(const char* buffer, Terminal* term) {
    if (match_cmd(buffer, "clear", 5)) {
        uint32_t *fb_ptr = (uint32_t *)term->fb->address;
        uint64_t total_pixels = (term->fb->pitch / 4) * term->fb->height;
        for (uint64_t i = 0; i < total_pixels; i++) {
            fb_ptr[i] = 0x000000;
        }
        term->cursor_y = 0;
    } 
    else if (match_cmd(buffer, "focus", 5)) {
        term->cursor_y += 8;
        term->cursor_x = 0;
        const char* msg = "Not implemented yet";
        for(int i=0; msg[i] != '\0'; i++) {
            term->draw_char(msg[i], term->cursor_x, term->cursor_y, 0xFF0000); // Red
            term->cursor_x += 8;
        }
    }
    else if (match_cmd(buffer, "help", 4)) {
        term->cursor_y += 8;
        term->cursor_x = 0;
        const char* msg = "Commands: clear, focus, help, 'load ui', doom";
        for(int i=0; msg[i] != '\0'; i++) {
            term->draw_char(msg[i], term->cursor_x, term->cursor_y, 0xFFFF00); // Yellow
            term->cursor_x += 8;
        }
    }
    else if (match_cmd(buffer, "load ui", 7)) {
        term->cursor_x = 0;
        const char* msg = "Loading UI...";
        for(int i=0; msg[i] != '\0'; i++) {
            term->draw_char(msg[i], term->cursor_x, term->cursor_y, 0xFF0000); // Red
            term->cursor_x += 8;
        }
        draw_launcher(term->fb);
    }
    else if (match_cmd(buffer, "doom", 4)) {
        term->cursor_y += 8;
        term->cursor_x = 0;
        const char* msg = "Entering DOOM...";
        for(int i=0; msg[i] != '\0'; i++) {
            term->draw_char(msg[i], term->cursor_x, term->cursor_y, 0xFF00FF);
            term->cursor_x += 8;
        }
        
        static char* argv[] = {(char*)"doom", (char*)"-iwad", (char*)"DOOM1.WAD", NULL};
        doomgeneric_Create(3, argv);
        
        while(1) {
            doomgeneric_Tick();
        }
    }
    else {
        term->cursor_y += 8;
        term->cursor_x = 0;
        const char* msg = "Unknown command";
        for(int i=0; msg[i] != '\0'; i++) {
            term->draw_char(msg[i], term->cursor_x, term->cursor_y, 0xFF0000); // Red
            term->cursor_x += 8;
        }
    }
}
