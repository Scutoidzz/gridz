#pragma once
#include <stdint.h>

struct MouseState {
    int x;
    int y;
    bool left_button;
    bool right_button;
    bool middle_button;
};

class Mouse {
public:
    static void init();
    static MouseState get_state();
    static void handle_interrupt();
    static void set_bounds(int width, int height);
private:
    static void wait_write();
    static void wait_read();
    static void write(uint8_t data);
    static uint8_t read();
};
