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
        for(int i=0; i<80*25; i++) {
            term->text_screen[i] = ' ';
        }
        term->char_x = 0;
        term->char_y = 0;
    } 
    else if (match_cmd(buffer, "help", 4)) {
        term->print("Kernel v0.5b - Commands:\n", 0x00FFFF);
        term->print("  clear    : Closes current terminal buffer\n");
        term->print("  neofetch : Display system info\n");
        term->print("  doom     : Launch Doom engine\n");
        term->print("  help     : Show this menu\n");
    }
    else if (match_cmd(buffer, "neofetch", 8)) {
        term->print("               ", 0x00FFFF); term->print("OS: Gridz OS x86_64\n");
        term->print("  ########     ", 0x00FFFF); term->print("Kernel: 0.5.2-alpha\n");
        term->print("  #      #     ", 0x00FFFF); term->print("Uptime: 2 mins\n");
        term->print("  #      #     ", 0x00FFFF); term->print("Shell: grid-sh 1.0\n");
        term->print("  ########     ", 0x00FFFF); term->print("Resolution: 800x600\n");
        term->print("               ", 0x00FFFF); term->print("Memory: 128MB / 1.2GB\n");
    }
    else if (match_cmd(buffer, "doom", 4)) {
        term->print("Initializing DOOM Subsystem...\n", 0xFF00FF);
        static char* argv[] = {(char*)"doom", (char*)"-iwad", (char*)"DOOM1.WAD", NULL};
        doomgeneric_Create(3, argv);
        while(1) {
            doomgeneric_Tick();
        }
    }
    else if (buffer[0] != '\0') {
        term->print("grid-sh: ", 0xFF0000);
        term->print(buffer);
        term->print(": command not found\n");
    }
}
