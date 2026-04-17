#include "doomgeneric.h"
#include "../../terminal.hpp"
#include "../../io.hpp"

// External access to the framebuffer
extern Terminal* global_term;

extern uint32_t* DG_ScreenBuffer;

#include "doomkeys.h"

extern "C" int get_next_key(int* pressed, unsigned char* key);
extern "C" uint32_t get_timer_ticks();

void DG_Init() {
    // Already initialized by kernel
}

void DG_DrawFrame() {
    if (!global_term || !global_term->fb) return;
    
    uint32_t* fb_ptr = (uint32_t*)global_term->fb->address;
    uint32_t pitch = global_term->fb->pitch / 4;

    for (int y = 0; y < DOOMGENERIC_RESY; y++) {
        for (int x = 0; x < DOOMGENERIC_RESX; x++) {
            fb_ptr[y * pitch + x] = DG_ScreenBuffer[y * DOOMGENERIC_RESX + x];
        }
    }
}

void DG_SleepMs(uint32_t ms) {
    uint32_t start = get_timer_ticks();
    while (get_timer_ticks() - start < ms) {
        __asm__("pause");
    }
}

uint32_t DG_GetTicksMs() {
    return get_timer_ticks();
}

int DG_GetKey(int* pressed, unsigned char* key) {
    unsigned char scancode;
    if (get_next_key(pressed, &scancode)) {
        switch (scancode) {
            case 0x48: *key = KEY_UPARROW; break;
            case 0x50: *key = KEY_DOWNARROW; break;
            case 0x4B: *key = KEY_LEFTARROW; break;
            case 0x4D: *key = KEY_RIGHTARROW; break;
            case 0x1C: *key = KEY_ENTER; break;
            case 0x01: *key = KEY_ESCAPE; break;
            case 0x1D: *key = KEY_FIRE; break;
            case 0x39: *key = KEY_USE; break;
            case 0x2A: *key = KEY_RSHIFT; break;
            default:
                if (scancode < 128) *key = kbd_map[scancode];
                else *key = 0;
                break;
        }
        return 1;
    }
    return 0;
}

void DG_SetWindowTitle(const char * title) {
}
