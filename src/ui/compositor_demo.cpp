#include "ui/compositor.hpp"
#include "ui/components/comp.hpp"

void demo_transparent_window(limine_framebuffer* fb) {
    Compositor::init();
    
    int window_id = Compositor::create_window(
        100, 100,              
        400, 300,              
        "Transparent Window",  
        0x404040,              
        180   
    );
    
    int window_id2 = Compositor::create_window(
        200, 200,
        350, 250,
        "Another Window",
        0x2A2A4A,
        220  // More opaque
    );
    Compositor::render(fb);
}
void demo_fade_window(limine_framebuffer* fb, int window_id) {
    for (int alpha = 0; alpha <= 255; alpha += 5) {
        Compositor::set_window_alpha(window_id, alpha);
        Compositor::render(fb);
    }
    
    for (int alpha = 255; alpha >= 0; alpha -= 5) {
        Compositor::set_window_alpha(window_id, alpha);
        Compositor::render(fb);
    }
}

// Example: Window management
void demo_window_management(limine_framebuffer* fb) {
    int win1 = Compositor::create_window(50, 50, 300, 200, "Window 1", 0x333333, 200);
    int win2 = Compositor::create_window(100, 100, 300, 200, "Window 2", 0x444444, 200);
    int win3 = Compositor::create_window(150, 150, 300, 200, "Window 3", 0x555555, 200);
    
    Compositor::focus_window(win2);
    Compositor::render(fb);
    
    Compositor::move_window(win1, 200, 50);
    Compositor::render(fb);
    
    Compositor::resize_window(win3, 400, 300);
    Compositor::render(fb);
    
    Compositor::set_window_visibility(win2, false);
    Compositor::render(fb);
    
=    Compositor::set_window_visibility(win2, true);
    Compositor::render(fb);
    
    Compositor::destroy_window(win1);
    Compositor::render(fb);
}
void demo_glass_window(limine_framebuffer* fb) {
    int glass_win = Compositor::create_window(
        150, 100,
        500, 350,
        "Glass Panel",
        0x1A1A2A,  
        120       
    );
    Compositor::render(fb);
}
void demo_integrated_ui(limine_framebuffer* fb) {
    draw_wallpaper(fb, 0x0D1117);
    
    int main_win = Compositor::create_window(
        100, 100,
        600, 400,
        "Main Applicationbu",
        0x2A2A3A,
        230
    );
    Compositor::render(fb);
    draw_taskbar(fb);
    draw_top_bar(fb);
}
