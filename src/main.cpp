#include <stdint.h>
#include <stddef.h>
#include "limine.h"
#include "io.hpp"
#include "terminal.hpp"
#include "ui/ui.hpp"
#include "idt.hpp"
#include "pic.hpp"

extern "C" void keyboard_isr();
extern "C" void timer_isr();
Terminal* global_term = nullptr;

struct key_event {
    bool pressed;
    uint8_t scancode;
};
static key_event key_queue[64];
static volatile int key_head = 0, key_tail = 0;

extern "C" void keyboard_handler() {
    uint8_t scancode = inb(0x60);
    int next = (key_head + 1) % 64;
    if (next != key_tail) {
        key_queue[key_head].pressed = !(scancode & 0x80);
        key_queue[key_head].scancode = scancode & 0x7F;
        key_head = next;
    }
    outb(0x20, 0x20);
}

extern "C" int get_next_key(int* pressed, unsigned char* key) {
    if (key_head == key_tail) return 0;
    *pressed = key_queue[key_tail].pressed ? 1 : 0;
    *key = key_queue[key_tail].scancode;
    key_tail = (key_tail + 1) % 64;
    return 1;
}

volatile uint32_t timer_ticks = 0;
extern "C" void timer_handler() {
    timer_ticks++;
    outb(0x20, 0x20);
}

extern "C" uint32_t get_timer_ticks() {
    return timer_ticks;
}

void pit_init(uint32_t frequency) {
    uint32_t divisor = 1193182 / frequency;
    outb(0x43, 0x36);
    outb(0x40, (uint8_t)(divisor & 0xFF));
    outb(0x40, (uint8_t)((divisor >> 8) & 0xFF));
}

bool in_gui_mode = false;


extern "C" {
    LIMINE_BASE_REVISION(2)
}

void itoa(int n, char* s, int base) {
    static const char digits[] = "0123456789ABCDEF";
    char* p = s;
    if (n == 0) { *p++ = '0'; *p = '\0'; return; }
    int i = n;
    while (i > 0) { *p++ = digits[i % base]; i /= base; }
    *p = '\0';
    // Reverse string
    char* q = s; p--;
    while (q < p) { char t = *q; *q = *p; *p = t; q++; p--; }
}

void serial_init() {
    outb(0x3F8 + 1, 0x00);
    outb(0x3F8 + 3, 0x80);
    outb(0x3F8 + 0, 0x03);
    outb(0x3F8 + 1, 0x00);
    outb(0x3F8 + 3, 0x03);
    outb(0x3F8 + 2, 0xC7);
    outb(0x3F8 + 4, 0x0B);
}

extern "C" void serial_putc(char c) {
    while ((inb(0x3F8 + 5) & 0x20) == 0);
    outb(0x3F8, c);
}

extern "C" void serial_print(const char* s) {
    while (*s) serial_putc(*s++);
}

static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0,
    .response = NULL
};

static volatile struct limine_module_request module_request = {
    .id = LIMINE_MODULE_REQUEST,
    .revision = 0,
    .response = NULL
};

extern "C" struct limine_module_response* get_module_response() {
    return module_request.response;
}

extern "C" void _start(void) {
    serial_init();
    serial_print("Serial Initialized\n");
    
    if (framebuffer_request.response == NULL || framebuffer_request.response->framebuffer_count < 1) {
        while (1) { __asm__("hlt"); }
    }

    struct limine_framebuffer *framebuffer = framebuffer_request.response->framebuffers[0];

    Terminal terminal(framebuffer);
    global_term = &terminal;
    terminal.print_prompt();

    auto mod_res = module_request.response;
    if (mod_res) {
        terminal.print("Modules found: ");
        char buf[10];
        itoa(mod_res->module_count, buf, 10);
        terminal.print(buf);
        terminal.print("\n");
        if (mod_res->module_count > 0) {
            terminal.print("First module: ");
            terminal.print(mod_res->modules[0]->path);
            terminal.print("\n");
        }
    } else {
        terminal.print("No module response found!\n");
    }

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

    pic_init();
    pit_init(1000);
    idt_set_descriptor(32, (void*)timer_isr, 0x8E);
    idt_set_descriptor(33, (void*)keyboard_isr, 0x8E);
    idtr_reg.limit = (uint16_t)(sizeof(idt_entry) * 256 - 1);
    idtr_reg.base  = (uint64_t)&idt[0];
    __asm__ volatile ("lidt %0" : : "m"(idtr_reg));
    __asm__ volatile ("sti");

    while (1) {
        if (in_gui_mode) {
            draw_launcher(framebuffer);
        }
        
        int pr; uint8_t sc;
        if (get_next_key(&pr, &sc)) { 
            if (pr) { 
                if (sc < 128 && kbd_map[sc]) {
                    char c = kbd_map[sc];
                    terminal.print_char(c);
                    if (c == '\n') terminal.print_prompt();
                }
            }
        }
    }
}