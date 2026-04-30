#include "mouse.hpp"
#include "io.hpp"

static volatile MouseState mouse_state = {100, 100, false, false, false};
static uint8_t mouse_cycle = 0;
static uint8_t mouse_packet[3];

void Mouse::wait_write() {
    uint32_t timeout = 100000;
    while (timeout-- && (inb(0x64) & 0x02));
}

void Mouse::wait_read() {
    uint32_t timeout = 100000;
    while (timeout-- && !(inb(0x64) & 0x01));
}

void Mouse::write(uint8_t data) {
    wait_write();
    outb(0x64, 0xD4);
    wait_write();
    outb(0x60, data);
}

uint8_t Mouse::read() {
    wait_read();
    return inb(0x60);
}

extern "C" void serial_print(const char* s);

void Mouse::init() {
    serial_print("Initializing PS/2 Mouse...\n");
    uint8_t status;

    wait_write();
    outb(0x64, 0xA8);

    wait_write();
    outb(0x64, 0x20);
    wait_read();
    status = inb(0x60) | 2;
    wait_write();
    outb(0x64, 0x60);
    wait_write();
    outb(0x60, status);

    write(0xF6);
    read();

    write(0xF4);
    read();
    
    serial_print("Mouse Initialized.\n");
}

static int screen_width = 800;
static int screen_height = 600;

void Mouse::set_bounds(int width, int height) {
    screen_width = width;
    screen_height = height;
}

void Mouse::handle_interrupt() {
    uint8_t status = inb(0x64);
    if (!(status & 0x20)) return;

    uint8_t data = inb(0x60);

    if (mouse_cycle == 0 && !(data & 0x08)) return; // Out of sync

    mouse_packet[mouse_cycle++] = data;

    if (mouse_cycle == 3) {
        mouse_cycle = 0;

        mouse_state.left_button = (mouse_packet[0] & 0x01);
        mouse_state.right_button = (mouse_packet[0] & 0x02);
        mouse_state.middle_button = (mouse_packet[0] & 0x04);

        int8_t rel_x = (int8_t)mouse_packet[1];
        int8_t rel_y = (int8_t)mouse_packet[2];

        // Sanity check for huge deltas?
        mouse_state.x += rel_x;
        mouse_state.y -= rel_y;

        if (mouse_state.x < 0) mouse_state.x = 0;
        if (mouse_state.y < 0) mouse_state.y = 0;
        if (mouse_state.x >= screen_width) mouse_state.x = screen_width - 1;
        if (mouse_state.y >= screen_height) mouse_state.y = screen_height - 1;
    }
}

MouseState Mouse::get_state() {
    MouseState ms;
    ms.x = mouse_state.x;
    ms.y = mouse_state.y;
    ms.left_button = mouse_state.left_button;
    ms.right_button = mouse_state.right_button;
    ms.middle_button = mouse_state.middle_button;
    return ms;
}
