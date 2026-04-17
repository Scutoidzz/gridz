#include <stdint.h>
#include <stddef.h>
#include "sboot.h"
#include "io.hpp"
#include "terminal.hpp"
#include "ui/ui.hpp"
#include "idt.hpp"
#include "pic.hpp"
#include "pmm.hpp"
#include "mouse.hpp"

extern "C" void keyboard_isr();
extern "C" void timer_isr();
extern "C" void mouse_isr();

extern "C" Terminal* global_term = nullptr;
static uint32_t* backbuffer = nullptr;

extern "C" void mouse_handler() {
    Mouse::handle_interrupt();
    outb(0xA0, 0x20);
    outb(0x20, 0x20);
}

struct key_event {
    bool pressed;
    uint8_t scancode;
};
static volatile key_event key_queue[256]; // Larger queue
static volatile int key_head = 0, key_tail = 0;

extern "C" void keyboard_handler() {
    uint8_t scancode = inb(0x60);
    int next = (key_head + 1) % 256;
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
    key_tail = (key_tail + 1) % 256;
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
    outb(0x40, divisor & 0xFF);
    outb(0x40, (divisor >> 8) & 0xFF);
}

extern "C" {
    sboot_BASE_REVISION(2)
}

void itoa(int n, char* s, int base) {
    static const char digits[] = "0123456789ABCDEF";
    int i = 0;
    if (n == 0) s[i++] = '0';
    else {
        while (n > 0) { s[i++] = digits[n % base]; n /= base; }
    }
    s[i] = '\0';
    int start = 0, end = i - 1;
    while(start < end) {
        char t = s[start]; s[start] = s[end]; s[end] = t;
        start++; end--;
    }
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

struct interrupt_frame {
    uint64_t r11, r10, r9, r8, rdi, rsi, rdx, rcx, rax;
    uint64_t vector, error_code, rip, cs, rflags, rsp, ss;
};

extern "C" void generic_exception_handler(interrupt_frame* frame) {
    (void)frame;
    serial_init();
    serial_print("FATAL EXCEPTION\n"); 
    while(1) { __asm__("hlt"); }
}

static volatile struct sboot_framebuffer_request framebuffer_request = {
    .id = sboot_FRAMEBUFFER_REQUEST,
    .revision = 0,
    .response = NULL
};

static volatile struct sboot_memmap_request memmap_request = {
    .id = sboot_MEMMAP_REQUEST,
    .revision = 0,
    .response = NULL
};

static volatile struct sboot_hhdm_request hhdm_request = {
    .id = sboot_HHDM_REQUEST,
    .revision = 0,
    .response = NULL
};

static volatile struct sboot_module_request module_request = {
    .id = sboot_MODULE_REQUEST,
    .revision = 0,
    .response = NULL,
    .internal_module_count = 0,
    .internal_modules = NULL
};

extern "C" struct sboot_module_response* get_module_response() {
    return module_request.response;
}

extern "C" void _start(void) {
    serial_init();
    
    if (framebuffer_request.response == NULL || framebuffer_request.response->framebuffer_count < 1) {
        while (1) { __asm__("hlt"); }
    }

    struct sboot_framebuffer *framebuffer = framebuffer_request.response->framebuffers[0];

    if (memmap_request.response && hhdm_request.response) {
        PMM::init(memmap_request.response, hhdm_request.response);
        uint64_t buffer_size = framebuffer->pitch * framebuffer->height;
        uint64_t pages_needed = (buffer_size + 4095) / 4096;
        backbuffer = (uint32_t*)((uint64_t)PMM::alloc_pages(pages_needed) + hhdm_request.response->offset);
    }
    
    if (!backbuffer) while(1);

    // Allocate terminal via PMM to be 100% safe (needs ~30KB)
    uint64_t term_pages = (sizeof(Terminal) + 4095) / 4096;
    Terminal* terminal = (Terminal*)((uint64_t)PMM::alloc_pages(term_pages) + hhdm_request.response->offset);
    
    // Explicitly initialize with zero to avoid garbage in pointers
    uint8_t* term_bytes = (uint8_t*)terminal;
    for(uint64_t i=0; i < term_pages * 4096; i++) term_bytes[i] = 0;

    terminal->fb = framebuffer;
    terminal->current_buffer = backbuffer;
    terminal->set_window(80, 80, 600, 300);
    global_term = terminal;

    pic_init(); 
    pit_init(1000);
    Mouse::init();
    Mouse::set_bounds(framebuffer->width, framebuffer->height);
    
    idt_set_descriptor(32, (void*)timer_isr, 0x8E);
    idt_set_descriptor(33, (void*)keyboard_isr, 0x8E);
    idt_set_descriptor(44, (void*)mouse_isr, 0x8E);
    idtr_reg.limit = (uint16_t)(sizeof(idt_entry) * 256 - 1);
    idtr_reg.base  = (uint64_t)&idt[0];
    __asm__ volatile ("lidt %0" : : "m"(idtr_reg));
    __asm__ volatile ("sti");

    bool last_mouse_btn = false;

    while (1) {
        uint64_t total_dwords = (framebuffer->pitch * framebuffer->height) / 4;
        for(uint64_t i=0; i < total_dwords; i++) backbuffer[i] = 0x444444;

        draw_launcher(framebuffer, backbuffer, terminal->is_open);
        
        if (terminal->is_open) {
            terminal->render();
        }
        
        MouseState ms = Mouse::get_state();
        if (ms.left_button && !last_mouse_btn) {
            if (ms.x >= 10 && ms.x <= 110 && ms.y >= (int)framebuffer->height - 35 && ms.y <= (int)framebuffer->height - 5) {
                terminal->is_open = !terminal->is_open;
                if (terminal->is_open) {
                    terminal->print("Welcome to Gridz OS v0.5b\n", 0x00FFFF);
                    terminal->print("Type 'help' for a list of commands.\n\n");
                    terminal->print_prompt();
                }
            }
        }
        last_mouse_btn = ms.left_button;

        draw_cursor(framebuffer, backbuffer, ms.x, ms.y);

        uint32_t* fb_ptr = (uint32_t*)framebuffer->address;
        for(uint64_t i=0; i < total_dwords; i++) fb_ptr[i] = backbuffer[i];

        int pr; uint8_t sc;
        if (get_next_key(&pr, &sc)) { 
            if (pr && terminal->is_open) { 
                if (sc < 128 && kbd_map[sc]) {
                    terminal->print_char(kbd_map[sc]);
                }
            }
        }
    }
}