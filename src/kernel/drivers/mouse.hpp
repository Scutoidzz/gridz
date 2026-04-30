#pragma once
#include <stdint.h>
#include "io.hpp"

// ── Global mouse state ────────────────────────────────────────────────────────
volatile int mouse_x = 400;
volatile int mouse_y = 300;
volatile uint8_t mouse_buttons = 0;

static int mouse_screen_w = 800;
static int mouse_screen_h = 600;

// ── PS/2 helper macros
static inline void mouse_wait_write() {
    int timeout = 100000;
    while (timeout-- && (inb(0x64) & 0x02));
}
static inline void mouse_wait_read() {
    int timeout = 100000;
    while (timeout-- && !(inb(0x64) & 0x01));
}
static inline void mouse_write(uint8_t val) {
    mouse_wait_write();
    outb(0x64, 0xD4);
    mouse_wait_write();
    outb(0x60, val);
}
static inline uint8_t mouse_read() {
    mouse_wait_read();
    return inb(0x60);
}

// ── Initialise the PS/2 mouse (call once, before STI) ────────────────────────
static inline void ps2_mouse_init(int screen_w, int screen_h) {
    mouse_screen_w = screen_w;
    mouse_screen_h = screen_h;
    mouse_x = screen_w / 2;
    mouse_y = screen_h / 2;

    // Enable auxiliary device
    mouse_wait_write();
    outb(0x64, 0xA8);

    // Enable IRQ12 in the controller command byte
    mouse_wait_write();
    outb(0x64, 0x20);
    mouse_wait_read();
    uint8_t status = inb(0x60);
    status |= 0x02;   // enable IRQ12
    status &= ~0x20;  // clear "disable mouse clock" bit
    mouse_wait_write();
    outb(0x64, 0x60);
    mouse_wait_write();
    outb(0x60, status);

    // Use default settings
    mouse_write(0xF6);
    mouse_read();   // ACK

    // Enable packet streaming
    mouse_write(0xF4);
    mouse_read();   // ACK
}

// ── Packet state machine ──────────────────────────────────────────────────────
static uint8_t  mouse_packet[3];
static int      mouse_cycle = 0;

// Call from your IRQ12 handler (C-linkage wrapper in main.cpp).
// NOTE: EOI is sent by the caller (mouse_handler in main.cpp) — do NOT send
// it here to avoid double-EOI which desynchronises the PIC chain.
static inline void mouse_handle_irq() {
    uint8_t status = inb(0x64);

    // Bit 0: output buffer has data.  Bit 5: data came from mouse (AUX).
    // If buffer is empty, nothing to do.
    if (!(status & 0x01)) return;

    // If bit 5 is clear, the byte is from the keyboard, not the mouse.
    // Drain it so it doesn't block future mouse bytes.
    if (!(status & 0x20)) {
        (void)inb(0x60);
        return;
    }

    uint8_t data = inb(0x60);

    switch (mouse_cycle) {
        case 0:
            // Bit 3 must always be 1 in the first packet byte; use it to
            // re-sync if we get out of step.
            if (!(data & 0x08)) { mouse_cycle = 0; break; }
            // Ignore packets with X/Y overflow flags set
            if (data & 0xC0)   { mouse_cycle = 0; break; }
            mouse_packet[0] = data;
            mouse_cycle = 1;
            break;
        case 1:
            mouse_packet[1] = data;
            mouse_cycle = 2;
            break;
        case 2: {
            mouse_packet[2] = data;
            mouse_cycle = 0;

            int dx = (int)mouse_packet[1] - ((mouse_packet[0] & 0x10) ? 256 : 0);
            int dy = (int)mouse_packet[2] - ((mouse_packet[0] & 0x20) ? 256 : 0);

            mouse_x += dx;
            mouse_y -= dy; // PS/2 Y is inverted

            if (mouse_x < 0) mouse_x = 0;
            if (mouse_y < 0) mouse_y = 0;
            if (mouse_x >= mouse_screen_w) mouse_x = mouse_screen_w - 1;
            if (mouse_y >= mouse_screen_h) mouse_y = mouse_screen_h - 1;

            mouse_buttons = mouse_packet[0] & 0x07;
            break;
        }
    }
    // EOI is sent by mouse_handler() in main.cpp — do not send here.
}
