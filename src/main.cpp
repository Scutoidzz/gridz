#include <stdint.h>
#include <stddef.h>
#include "limine.h"
#include "io.hpp"
#include "terminal.hpp"


extern "C" {
    LIMINE_BASE_REVISION(2)
}

static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0,
    .response = NULL
};

extern "C" void _start(void) {
    
    if (framebuffer_request.response == NULL || framebuffer_request.response->framebuffer_count < 1) {
        while (1) { __asm__("hlt"); }
    }

    struct limine_framebuffer *framebuffer = framebuffer_request.response->framebuffers[0];

    Terminal terminal(framebuffer);
    terminal.print_prompt();

    terminal.draw_char('G', 0, 0, 0xFFFFFF);
    terminal.draw_char('r', 8, 0, 0xFFFFFF);
    terminal.draw_char('i', 16, 0, 0xFFFFFF);
    terminal.draw_char('d', 24, 0, 0xFFFFFF);
    terminal.draw_char('z', 32, 0, 0xFFFFFF);

    terminal.draw_char('-', 48, 0, 0xFFFFFF);

    terminal.draw_char('P', 64, 0, 0xFFFFFF);
    terminal.draw_char('r', 72, 0, 0xFFFFFF);
    terminal.draw_char('e', 80, 0, 0xFFFFFF);
    terminal.draw_char('s', 88, 0, 0xFFFFFF);
    terminal.draw_char('s', 96, 0, 0xFFFFFF);
    terminal.draw_char(' ', 104, 0, 0xFFFFFF);
    terminal.draw_char('E', 112, 0, 0xFFFFFF);
    terminal.draw_char('n', 120, 0, 0xFFFFFF);
    terminal.draw_char('t', 128, 0, 0xFFFFFF);
    terminal.draw_char('e', 136, 0, 0xFFFFFF);
    terminal.draw_char('r', 144, 0, 0xFFFFFF);
    terminal.draw_char(' ', 152, 0, 0xFFFFFF);
    terminal.draw_char('t', 160, 0, 0xFFFFFF);
    terminal.draw_char('o', 168, 0, 0xFFFFFF);
    terminal.draw_char(' ', 176, 0, 0xFFFFFF);
    terminal.draw_char('s', 184, 0, 0xFFFFFF);
    terminal.draw_char('t', 192, 0, 0xFFFFFF);
    terminal.draw_char('a', 200, 0, 0xFFFFFF);
    terminal.draw_char('r', 208, 0, 0xFFFFFF);
    terminal.draw_char('t', 216, 0, 0xFFFFFF);
    
    terminal.cursor_x = 224;

    while (1) {
        if (inb(0x64) & 1) { 
            volatile uint8_t scancode = inb(0x60); 
            if (scancode < 0x80 && kbd_map[scancode]) { 
                char c = kbd_map[scancode];
                
                terminal.print_char(c);

                if (c == '\n') {
                    terminal.print_prompt();
                }
            }
        }
    }
}