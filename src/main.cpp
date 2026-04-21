/*
 * main.cpp — Gridz OS kernel entry point
 *
 * Responsibilities:
 *   - Hardware init (serial, PIC, PIT, IDT, PS/2 mouse)
 *   - Keyboard and timer ISR handlers
 *   - Main event loop that dispatches to either GUI or terminal mode
 */

#include <stdint.h>
#include <stddef.h>
#include "limine.h"
#include "io.hpp"
#include "terminal.hpp"
#include "ui/ui.hpp"
#include "idt.hpp"
#include "pic.hpp"
#include "drivers/mouse.hpp"
#include "doom/doomgeneric/doomgeneric.h"

/* ── Assembly ISR stubs (interrups.asm) ────────────────────────────────────── */
extern "C" void keyboard_isr();
extern "C" void timer_isr();
extern "C" void mouse_isr();
extern "C" void exc0(); extern "C" void exc1(); extern "C" void exc2(); extern "C" void exc3();
extern "C" void exc4(); extern "C" void exc5(); extern "C" void exc6(); extern "C" void exc7();
extern "C" void exc8(); extern "C" void exc9(); extern "C" void exc10(); extern "C" void exc11();
extern "C" void exc12(); extern "C" void exc13(); extern "C" void exc14(); extern "C" void exc15();
extern "C" void exc16(); extern "C" void exc17(); extern "C" void exc18(); extern "C" void exc19();
extern "C" void exc20(); extern "C" void exc21(); extern "C" void exc22(); extern "C" void exc23();
extern "C" void exc24(); extern "C" void exc25(); extern "C" void exc26(); extern "C" void exc27();
extern "C" void exc28(); extern "C" void exc29(); extern "C" void exc30(); extern "C" void exc31();

/* ── Global state ──────────────────────────────────────────────────────────── */
extern Terminal* global_term;
extern "C" void serial_print(const char* s);
extern "C" void itoa(int n, char* s, int base);

struct cpu_status_t {
    uint64_t rax, rcx, rdx, rsi, rdi, r8, r9, r10, r11, rbx, rbp, r12, r13, r14, r15;
    uint64_t vector, error_code, rip, cs, rflags, rsp, ss;
};

extern "C" void exception_handler(cpu_status_t* status) {
    uint64_t cr2;
    __asm__ volatile("mov %%cr2, %0" : "=r"(cr2));

    serial_print("\nEXCEPTION: ");
    char buf[32];
    itoa(status->vector, buf, 10);
    serial_print(buf);
    serial_print(" RIP: ");
    itoa(status->rip, buf, 16);
    serial_print(buf);
    serial_print(" CR2: ");
    itoa(cr2, buf, 16);
    serial_print(buf);
    serial_print(" ERR: ");
    itoa(status->error_code, buf, 16);
    serial_print(buf);
    serial_print("\n");

    if (global_term) {
        global_term->print("\n*** KERNEL PANIC ***\n", 0xFF0000);
        global_term->print("Exception: ", 0xFF0000);
        itoa(status->vector, buf, 10);
        global_term->print(buf);
        global_term->print("\nFaulting Addr (CR2): ", 0xFFFFFF);
        itoa(cr2, buf, 16);
        global_term->print(buf);
        global_term->print("\nRIP: ", 0xFFFFFF);
        itoa(status->rip, buf, 16);
        global_term->print(buf);
    }

    while(1) { __asm__("cli; hlt"); }
}

/* ── Global state ──────────────────────────────────────────────────────────── */
Terminal* global_term  = nullptr;
bool      in_gui_mode  = true;   // start in GUI
bool      launch_doom  = false;  // set by start-menu click, consumed by loop
bool      doom_running = false;

/* ── Keyboard ring buffer ──────────────────────────────────────────────────── */
struct KeyEvent { bool pressed; uint8_t scancode; };
static KeyEvent       key_queue[64];
static volatile int   key_head = 0;
static volatile int   key_tail = 0;

__attribute__((target("general-regs-only")))
extern "C" void keyboard_handler() {
    uint8_t sc = inb(0x60);
    int next = (key_head + 1) % 64;
    if (next != key_tail) {
        key_queue[key_head].pressed  = !(sc & 0x80);
        key_queue[key_head].scancode = sc & 0x7F;
        key_head = next;
    }
    outb(0x20, 0x20);
}

extern "C" int get_next_key(int* pressed, unsigned char* key) {
    if (key_head == key_tail) return 0;
    *pressed = key_queue[key_tail].pressed ? 1 : 0;
    *key     = key_queue[key_tail].scancode;
    key_tail = (key_tail + 1) % 64;
    return 1;
}

/* ── Timer ─────────────────────────────────────────────────────────────────── */
volatile uint32_t timer_ticks = 0;

__attribute__((target("general-regs-only")))
extern "C" void timer_handler() {
    timer_ticks++;
    outb(0x20, 0x20);
}

extern "C" uint32_t get_timer_ticks() {
    return timer_ticks;
}

/* ── Mouse IRQ12 handler ───────────────────────────────────────────────────── */
__attribute__((target("general-regs-only")))
extern "C" void mouse_handler() {
    mouse_handle_irq();   // sends both PIC EOIs internally
}

/* ── PIT init ──────────────────────────────────────────────────────────────── */
static void pit_init(uint32_t freq) {
    uint32_t div = 1193182 / freq;
    outb(0x43, 0x36);
    outb(0x40, (uint8_t)(div & 0xFF));
    outb(0x40, (uint8_t)((div >> 8) & 0xFF));
}

/* ── Limine protocol ───────────────────────────────────────────────────────── */
extern "C" { LIMINE_BASE_REVISION(2) }

static volatile struct limine_framebuffer_request fb_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST, .revision = 0, .response = NULL
};
static volatile struct limine_module_request mod_request = {
    .id = LIMINE_MODULE_REQUEST, .revision = 0, .response = NULL
};

extern "C" struct limine_module_response* get_module_response() {
    return mod_request.response;
}

/* ── Utilities ─────────────────────────────────────────────────────────────── */
void itoa(int n, char* s, int base) {
    static const char digits[] = "0123456789ABCDEF";
    char* p = s;
    if (n == 0) { *p++ = '0'; *p = '\0'; return; }
    int i = n;
    while (i > 0) { *p++ = digits[i % base]; i /= base; }
    *p = '\0';
    char* q = s; p--;
    while (q < p) { char t = *q; *q = *p; *p = t; q++; p--; }
}

/* ── Serial ────────────────────────────────────────────────────────────────── */
void serial_init() {
    outb(0x3F8 + 1, 0x00); outb(0x3F8 + 3, 0x80);
    outb(0x3F8 + 0, 0x03); outb(0x3F8 + 1, 0x00);
    outb(0x3F8 + 3, 0x03); outb(0x3F8 + 2, 0xC7);
    outb(0x3F8 + 4, 0x0B);
}

extern "C" void serial_putc(char c) {
    while ((inb(0x3F8 + 5) & 0x20) == 0);
    outb(0x3F8, c);
}

extern "C" void serial_print(const char* s) {
    while (*s) serial_putc(*s++);
}

/* ── Helper: clear screen to black and reset terminal ──────────────────────── */
static void switch_to_terminal(limine_framebuffer* fb, Terminal* term) {
    uint32_t* ptr    = (uint32_t*)fb->address;
    uint64_t  pixels = (fb->pitch / 4) * fb->height;
    for (uint64_t i = 0; i < pixels; i++) ptr[i] = 0x000000;

    term->cursor_x = 0;
    term->cursor_y = 0;
    term->cmd_len  = 0;
    term->print("Gridz OS Terminal\n");
    term->print("Type 'help' for commands, 'load ui' to return to GUI.\n");
    term->print_prompt();
}

/* ══════════════════════════════════════════════════════════════════════════════
 * KERNEL ENTRY POINT
 * ══════════════════════════════════════════════════════════════════════════════ */
extern "C" void _start(void) {
    serial_init();
    serial_print("Serial Initialized\n");

    /* Enable FPU/SSE */
    uint64_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 &= ~(1 << 2); // clear TS
    cr0 |= (1 << 1);  // set MP
    __asm__ volatile("mov %0, %%cr0" : : "r"(cr0));
    
    uint64_t cr4;
    __asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= (3 << 9); // OSFXSR | OSXMMEXCPT
    __asm__ volatile("mov %0, %%cr4" : : "r"(cr4));

    /* Framebuffer sanity check */
    if (!fb_request.response || fb_request.response->framebuffer_count < 1) {
        while (1) __asm__("hlt");
    }
    struct limine_framebuffer* fb = fb_request.response->framebuffers[0];

    /* Terminal (used in text mode) */
    Terminal terminal(fb);
    global_term = &terminal;

    /* Hardware init */
    pic_init();
    pit_init(1000);
    
    void (*excs[])() = {
        exc0, exc1, exc2, exc3, exc4, exc5, exc6, exc7,
        exc8, exc9, exc10, exc11, exc12, exc13, exc14, exc15,
        exc16, exc17, exc18, exc19, exc20, exc21, exc22, exc23,
        exc24, exc25, exc26, exc27, exc28, exc29, exc30, exc31
    };
    for (int i = 0; i < 32; i++) idt_set_descriptor(i, (void*)excs[i], 0x8E);

    idt_set_descriptor(32, (void*)timer_isr,    0x8E);
    idt_set_descriptor(33, (void*)keyboard_isr, 0x8E);
    idt_set_descriptor(44, (void*)mouse_isr,    0x8E);  // IRQ12
    idtr_reg.limit = (uint16_t)(sizeof(idt_entry) * 256 - 1);
    idtr_reg.base  = (uint64_t)&idt[0];
    __asm__ volatile("lidt %0" : : "m"(idtr_reg));
    ps2_mouse_init((int)fb->width, (int)fb->height);
    __asm__ volatile("sti");

    /* Boot into GUI */
    init_ui(fb);

    /* ── Main event loop ───────────────────────────────────────────────────── */
    uint32_t last_tick  = 0;
    int      last_mx    = -1;
    int      last_my    = -1;
    uint8_t  prev_btns  = 0;
    bool     was_gui    = true;   // track mode transitions

    while (1) {
        __asm__ volatile("hlt");  // sleep until next interrupt

        uint32_t now = get_timer_ticks();

        /* ── GUI mode ──────────────────────────────────────────────────────── */
        if (in_gui_mode) {
            /* If we just entered GUI mode, redraw everything */
            if (!was_gui) {
                init_ui(fb);
                last_mx = -1;
                last_my = -1;
                was_gui = true;
            }

            /* Refresh bars + clock at ~10 Hz */
            if (now - last_tick >= 100) {
                draw_top_bar(fb);
                draw_taskbar(fb);
                draw_clock_window(fb);   // no-op when not open
                last_tick = now;
            }

            /* Cursor: only redraw when mouse moves */
            int mx = mouse_x;
            int my = mouse_y;
            if (mx != last_mx || my != last_my) {
                draw_ui_cursor(fb);
                last_mx = mx;
                last_my = my;
            }

            /* Click: left-button leading edge */
            uint8_t btns    = mouse_buttons;
            uint8_t clicked = (uint8_t)(~prev_btns & btns);
            prev_btns = btns;

            if (clicked & 0x01) {
                draw_ui_cursor(fb);           // restore bg
                handle_ui_click(fb, mx, my);
                draw_ui_cursor(fb);           // redraw on new state
            }

            /* Doom launch (requested via start menu) */
            if (launch_doom) {
                launch_doom  = false;
                in_gui_mode  = false;
                was_gui      = false;
                doom_running = true;
                static char* argv[] = {
                    (char*)"doom", (char*)"-iwad", (char*)"DOOM1.WAD", nullptr
                };
                doomgeneric_Create(3, argv);
                while (doom_running) {
                    doomgeneric_Tick();
                }
                
                // Return to GUI
                in_gui_mode = true;
                was_gui = false;
            }

        /* ── Terminal mode ─────────────────────────────────────────────────── */
        } else {
            /* If we just left GUI mode, set up the terminal screen */
            if (was_gui) {
                switch_to_terminal(fb, &terminal);
                was_gui = false;
            }

            /* Feed keyboard input to the terminal */
            int pressed;
            uint8_t sc;
            if (get_next_key(&pressed, &sc)) {
                if (pressed && sc < 128 && kbd_map[sc]) {
                    char c = kbd_map[sc];
                    terminal.print_char(c);
                    if (c == '\n') terminal.print_prompt();
                }
            }
        }
    }
}