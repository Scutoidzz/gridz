#include "compositor.hpp"
#include "components/comp.hpp"
#include "../lib/string.hpp"

Window Compositor::windows[MAX_WINDOWS];
int Compositor::window_count = 0;
int Compositor::focused_window = -1;
int Compositor::dragging_window = -1;
int Compositor::drag_offset_x = 0;
int Compositor::drag_offset_y = 0;
int Compositor::resizing_window = -1;
int Compositor::resize_start_mx = 0;
int Compositor::resize_start_my = 0;
int Compositor::resize_start_w = 0;
int Compositor::resize_start_h = 0;

void Compositor::init() {
    for (int i = 0; i < MAX_WINDOWS; i++) {
        windows[i].visible = false;
        windows[i].alpha = 255;
        windows[i].z_index = 0;
    }
    dragging_window = -1;
}

void Compositor::sort_by_zindex() {
    // Simple bubble sort by z_index (lower z_index = render first = background)
    for (int i = 0; i < window_count - 1; i++) {
        for (int j = 0; j < window_count - i - 1; j++) {
            if (windows[j].z_index > windows[j + 1].z_index) {
                Window temp = windows[j];
                windows[j] = windows[j + 1];
                windows[j + 1] = temp;
            }
        }
    }
}

int Compositor::create_window(int x, int y, int width, int height, const char* title, uint32_t bg_color, uint8_t alpha, int z_index) {
    if (window_count >= MAX_WINDOWS) return -1;
    
    int id = window_count;
    windows[id].x = x;
    windows[id].y = y;
    windows[id].width = width;
    windows[id].height = height;
    windows[id].background_color = bg_color;
    windows[id].alpha = alpha;
    windows[id].visible = true;
    windows[id].focused = false;
    windows[id].title = title;
    windows[id].z_index = z_index;
    windows[id].content_buffer = nullptr;
    windows[id].content_buffer_w = 0;
    windows[id].content_buffer_h = 0;

    window_count++;
    focus_window(id);
    return id;
}

void Compositor::destroy_window(int window_id) {
    if (window_id < 0 || window_id >= window_count) return;
    
    // Shift remaining windows
    for (int i = window_id; i < window_count - 1; i++) {
        windows[i] = windows[i + 1];
    }
    
    window_count--;
    if (focused_window >= window_count) {
        focused_window = window_count - 1;
    }
}

void Compositor::move_window(int window_id, int x, int y) {
    if (window_id < 0 || window_id >= window_count) return;
    windows[window_id].x = x;
    windows[window_id].y = y;
}

void Compositor::resize_window(int window_id, int width, int height) {
    if (window_id < 0 || window_id >= window_count) return;
    windows[window_id].width = width;
    windows[window_id].height = height;
}

void Compositor::set_window_alpha(int window_id, uint8_t alpha) {
    if (window_id < 0 || window_id >= window_count) return;
    windows[window_id].alpha = alpha;
}

void Compositor::set_window_zindex(int window_id, int z_index) {
    if (window_id < 0 || window_id >= window_count) return;
    windows[window_id].z_index = z_index;
}

void Compositor::focus_window(int window_id) {
    if (window_id < 0 || window_id >= window_count) return;
    
    // Unfocus previous
    if (focused_window >= 0 && focused_window < window_count) {
        windows[focused_window].focused = false;
    }
    
    windows[window_id].focused = true;
    focused_window = window_id;
}

void Compositor::set_window_visibility(int window_id, bool visible) {
    if (window_id < 0 || window_id >= window_count) return;
    windows[window_id].visible = visible;
}

// Dragging support
bool Compositor::start_drag(int window_id, int mx, int my) {
    if (window_id < 0 || window_id >= window_count) return false;
    if (!windows[window_id].visible) return false;
    
    dragging_window = window_id;
    drag_offset_x = mx - windows[window_id].x;
    drag_offset_y = my - windows[window_id].y;
    return true;
}

void Compositor::update_drag(int mx, int my) {
    if (dragging_window < 0 || dragging_window >= window_count) return;
    
    windows[dragging_window].x = mx - drag_offset_x;
    windows[dragging_window].y = my - drag_offset_y;
}

void Compositor::end_drag() {
    dragging_window = -1;
}

bool Compositor::is_dragging() {
    return dragging_window >= 0;
}

bool Compositor::start_resize(int window_id, int mx, int my) {
    if (window_id < 0 || window_id >= window_count) return false;
    resizing_window = window_id;
    resize_start_mx = mx;
    resize_start_my = my;
    resize_start_w = windows[window_id].width;
    resize_start_h = windows[window_id].height;
    return true;
}

void Compositor::update_resize(int mx, int my) {
    if (resizing_window < 0 || resizing_window >= window_count) return;
    int new_w = resize_start_w + (mx - resize_start_mx);
    int new_h = resize_start_h + (my - resize_start_my);
    if (new_w < 120) new_w = 120;
    if (new_h < 80) new_h = 80;
    windows[resizing_window].width = new_w;
    windows[resizing_window].height = new_h;
}

void Compositor::end_resize() {
    resizing_window = -1;
}

bool Compositor::is_resizing() {
    return resizing_window >= 0;
}

bool Compositor::is_on_resize_handle(int window_id, int mx, int my) {
    if (window_id < 0 || window_id >= window_count) return false;
    Window* win = &windows[window_id];
    return (mx >= win->x + win->width - 14 && mx <= win->x + win->width &&
            my >= win->y + win->height - 14 && my <= win->y + win->height);
}

int Compositor::get_window_at(int mx, int my) {
    // Check from top to bottom (reverse order)
    for (int i = window_count - 1; i >= 0; i--) {
        if (!windows[i].visible) continue;
        
        Window* win = &windows[i];
        if (mx >= win->x && mx <= win->x + win->width &&
            my >= win->y && my <= win->y + win->height) {
            return i;
        }
    }
    return -1;
}

bool Compositor::is_on_titlebar(int window_id, int mx, int my) {
    if (window_id < 0 || window_id >= window_count) return false;
    
    Window* win = &windows[window_id];
    return (mx >= win->x && mx <= win->x + win->width &&
            my >= win->y && my <= win->y + MAX_TITLEBAR_HEIGHT);
}

void Compositor::draw_reflective_titlebar(limine_framebuffer* fb, Window* win, bool focused) {
    if (!fb || !win) return;

    int titlebar_y = win->y;
    int titlebar_height = MAX_TITLEBAR_HEIGHT;

    // macOS-style: light gray gradient titlebar
    uint8_t top_r = focused ? 0xD8 : 0xED;
    uint8_t top_g = focused ? 0xD8 : 0xED;
    uint8_t top_b = focused ? 0xD8 : 0xED;
    uint8_t bot_r = focused ? 0xBE : 0xE0;
    uint8_t bot_g = focused ? 0xBE : 0xE0;
    uint8_t bot_b = focused ? 0xBE : 0xE0;

    for (int row = 0; row < titlebar_height; row++) {
        uint8_t r = (uint8_t)((int)top_r + ((int)bot_r - top_r) * row / titlebar_height);
        uint8_t g = (uint8_t)((int)top_g + ((int)bot_g - top_g) * row / titlebar_height);
        uint8_t b = (uint8_t)((int)top_b + ((int)bot_b - top_b) * row / titlebar_height);
        uint32_t row_color = ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
        draw_rectangle_alpha(fb, win->x, titlebar_y + row, win->width, 1, row_color, win->alpha);
    }

    // Draw close button (red circle)
    int close_x = win->x + win->width - 25;
    int close_y = titlebar_y + 8;
    draw_circle(fb, close_x, close_y, 8, 0xCC3333);
    draw_string(fb, close_x - 3, close_y - 4, "X", 0xFFFFFF);

    if (win->title) {
        int text_x = win->x + 10;
        int text_y = titlebar_y + 10;
        uint32_t title_color = focused ? 0x222222 : 0x555555;
        draw_string(fb, text_x, text_y, win->title, title_color);
    }
}

void Compositor::draw_titlebar(limine_framebuffer* fb, Window* win, bool focused) {
    draw_reflective_titlebar(fb, win, focused);
}

void Compositor::render(limine_framebuffer* fb) {
    if (!fb) return;

    // Build a z-sorted index array so window IDs stay stable
    int order[MAX_WINDOWS];
    for (int i = 0; i < window_count; i++) order[i] = i;
    for (int i = 0; i < window_count - 1; i++)
        for (int j = 0; j < window_count - i - 1; j++)
            if (windows[order[j]].z_index > windows[order[j+1]].z_index) {
                int tmp = order[j]; order[j] = order[j+1]; order[j+1] = tmp;
            }

    for (int ii = 0; ii < window_count; ii++) {
        int i = order[ii];
        if (!windows[i].visible) continue;

        Window* win = &windows[i];
        
        // Draw window background with alpha or custom content_buffer
        if (win->content_buffer) {
            uint32_t* fb_ptr = (uint32_t*)fb->address;
            int stride = fb->pitch / 4;
            int buf_w = win->content_buffer_w ? win->content_buffer_w : win->width;
            int buf_h = win->content_buffer_h ? win->content_buffer_h : (win->height - MAX_TITLEBAR_HEIGHT);
            int draw_w = win->width < buf_w ? win->width : buf_w;
            int draw_h = (win->height - MAX_TITLEBAR_HEIGHT) < buf_h ? (win->height - MAX_TITLEBAR_HEIGHT) : buf_h;
            int start_x = win->x;
            int start_y = win->y + MAX_TITLEBAR_HEIGHT;

            for (int y = 0; y < draw_h; y++) {
                if (start_y + y < 0 || start_y + y >= (int)fb->height) continue;
                for (int x = 0; x < draw_w; x++) {
                    if (start_x + x < 0 || start_x + x >= (int)fb->width) continue;
                    fb_ptr[(start_y + y) * stride + (start_x + x)] = win->content_buffer[y * buf_w + x];
                }
            }
        } else {
            draw_rectangle_alpha(fb, win->x, win->y + MAX_TITLEBAR_HEIGHT, 
                                win->width, win->height - MAX_TITLEBAR_HEIGHT, 
                                win->background_color, win->alpha);
        }
        
        // Draw titlebar
        draw_titlebar(fb, win, win->focused);
        
        // macOS-style subtle border
        uint32_t border_color = win->focused ? 0xB0B0B0 : 0xD0D0D0;
        draw_rectangle_alpha(fb, win->x, win->y, win->width, 1, border_color, win->alpha);
        draw_rectangle_alpha(fb, win->x, win->y + win->height - 1, win->width, 1, border_color, win->alpha);
        draw_rectangle_alpha(fb, win->x, win->y, 1, win->height, border_color, win->alpha);
        draw_rectangle_alpha(fb, win->x + win->width - 1, win->y, 1, win->height, border_color, win->alpha);

        // Resize handle — 3 dots in bottom-right corner
        uint32_t rc = win->focused ? 0x888888 : 0xAAAAAA;
        int rx = win->x + win->width;
        int ry = win->y + win->height;
        draw_rectangle_alpha(fb, rx - 4,  ry - 4,  2, 2, rc, win->alpha);
        draw_rectangle_alpha(fb, rx - 8,  ry - 4,  2, 2, rc, win->alpha);
        draw_rectangle_alpha(fb, rx - 4,  ry - 8,  2, 2, rc, win->alpha);
    }
}
