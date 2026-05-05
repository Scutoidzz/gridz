#pragma once
#include "limine.h"

void draw_rect(limine_framebuffer* fb, int x, int y, int width, int height, uint32_t color);
void draw_launcher(limine_framebuffer* fb);

void init_ui(limine_framebuffer* fb);
void draw_taskbar(limine_framebuffer* fb);
void draw_top_bar(limine_framebuffer* fb);
void draw_ui_cursor(limine_framebuffer* fb);
void draw_start_menu(limine_framebuffer* fb);
void draw_neofetch(limine_framebuffer* fb);
void draw_clock_window(limine_framebuffer* fb);
void draw_calculator_window(limine_framebuffer* fb);
void draw_filemanager_window(limine_framebuffer* fb);
void draw_wallpaper_gradient(limine_framebuffer* fb);
bool handle_ui_click(limine_framebuffer* fb, int mx, int my);
void ui_cursor_invalidate();

extern bool installer_available;

extern bool mouse_dragging;
extern bool mouse_resizing;
void handle_mouse_drag(int mx, int my);
void end_mouse_drag();
void handle_mouse_resize(int mx, int my);
void end_mouse_resize();
