#pragma once
#include "limine.h"
#include <stdint.h>

#define MAX_WINDOWS 12
#define MAX_TITLEBAR_HEIGHT 32

typedef struct {
    int x, y;
    int width, height;
    uint32_t background_color;
    uint8_t alpha;
    bool visible;
    bool focused;
    const char* title;
    int z_index;
    uint32_t* content_buffer;
    int content_buffer_w; // actual buffer dimensions (0 = use window size)
    int content_buffer_h;
} Window;

class Compositor {
public:
    static void init();
    static int create_window(int x, int y, int width, int height, const char* title, uint32_t bg_color, uint8_t alpha, int z_index = 0);
    static void destroy_window(int window_id);
    static void move_window(int window_id, int x, int y);
    static void resize_window(int window_id, int width, int height);
    static void set_window_alpha(int window_id, uint8_t alpha);
    static void set_window_zindex(int window_id, int z_index);
    static void focus_window(int window_id);
    static void set_window_visibility(int window_id, bool visible);
    static void render(limine_framebuffer* fb);
    static void draw_titlebar(limine_framebuffer* fb, Window* win, bool focused);
    static void draw_reflective_titlebar(limine_framebuffer* fb, Window* win, bool focused);
    
    // Dragging support
    static bool start_drag(int window_id, int mx, int my);
    static void update_drag(int mx, int my);
    static void end_drag();
    static bool is_dragging();
    static int get_window_at(int mx, int my);
    static bool is_on_titlebar(int window_id, int mx, int my);

    // Resize support
    static bool start_resize(int window_id, int mx, int my);
    static void update_resize(int mx, int my);
    static void end_resize();
    static bool is_resizing();
    static bool is_on_resize_handle(int window_id, int mx, int my);

    static Window windows[MAX_WINDOWS];
    static int window_count;

private:
    static int focused_window;
    static int dragging_window;
    static int drag_offset_x;
    static int drag_offset_y;
    static int resizing_window;
    static int resize_start_mx;
    static int resize_start_my;
    static int resize_start_w;
    static int resize_start_h;
    static void sort_by_zindex();
};
