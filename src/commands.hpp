#pragma once
#include "ui/ui.hpp"

inline bool match_cmd(const char* buf, const char* target, int target_len) {
    for (int i = 0; i < target_len; i++) {
        if (buf[i] != target[i]) return false;
    }
    return buf[target_len] == '\0';
}

#include "doom/doomgeneric/doomgeneric.h"
#include "drivers/ata.hpp"
#include "fs/fat32.hpp"

extern bool in_gui_mode;

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
        in_gui_mode = true;
    }
    else if (match_cmd(buffer, "diskinfo", 8)) {
        term->cursor_y += 8;
        term->cursor_x = 0;
        bool master = ata::exists(false);
        bool slave  = ata::exists(true);
        
        if (master) {
            if (fat32::init(false)) term->print("Master: FAT32 Partition detected.\n");
            else term->print("Master: Found (No FAT32).\n");
        } else {
            term->print("Master: Not found.\n");
        }
        
        if (slave) {
            if (fat32::init(true)) term->print("Slave:  FAT32 Partition detected.\n");
            else term->print("Slave:  Found (No FAT32).\n");
        } else {
            term->print("Slave:  Not found.\n");
        }
        
        if (!master && !slave) {
            term->print("No IDE disks detected. Check QEMU config.\n");
        }
    }
    else if (match_cmd(buffer, "install", 7)) {
        term->cursor_y += 8;
        term->cursor_x = 0;
        term->print("Installing to Master... ");
        if (fat32::format(131072, "GRIDZ_OS", false)) {
            term->print("Success.\n");
        } else {
            term->print("Failed.\n");
            term->print("Installing to Slave... ");
            if (fat32::format(131072, "GRIDZ_OS", true)) {
                term->print("Success.\n");
            } else {
                term->print("Fatal: No writable disk found.\n");
            }
        }
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
        const char* msg = "Command not found";
        for(int i=0; msg[i] != '\0'; i++) {
            term->draw_char(msg[i], term->cursor_x, term->cursor_y, 0xFF0000); // Red
            term->cursor_x += 8;
        }
    }
}
