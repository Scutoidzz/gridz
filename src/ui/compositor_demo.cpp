#include "ui/compositor.hpp"
#include "ui/components/comp.hpp"

void demo_transparent_window(limine_framebuffer* fb) {
    Compositor::init();
    
    int window_id = Compositor::create_window(
        100, 100,              // x, y position
        400, 300,              // width, height
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
    
    // Render all windows (compositor handles layering and transparency)
    Compositor::render(fb);
}

// Example: Dynamic transparency adjustment
void demo_fade_window(limine_framebuffer* fb, int window_id) {
    // Fade window in
    for (int alpha = 0; alpha <= 255; alpha += 5) {
        Compositor::set_window_alpha(window_id, alpha);
        Compositor::render(fb);
        // In a real system, you'd have a delay here
    }
    
    // Fade window out
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
    
    // Focus window 2 (brings it to top)
    Compositor::focus_window(win2);
    Compositor::render(fb);
    
    // Move window 1
    Compositor::move_window(win1, 200, 50);
    Compositor::render(fb);
    
    // Resize window 3
    Compositor::resize_window(win3, 400, 300);
    Compositor::render(fb);
    
    // Hide window 2
    Compositor::set_window_visibility(win2, false);
    Compositor::render(fb);
    
    // Show window 2 again
    Compositor::set_window_visibility(win2, true);
    Compositor::render(fb);
    
    // Close window 1
    Compositor::destroy_window(win1);
    Compositor::render(fb);
}

// Example: Glass-like effect (very transparent with dark background)
void demo_glass_window(limine_framebuffer* fb) {
    int glass_win = Compositor::create_window(
        150, 100,
        500, 350,
        "Glass Panel",
        0x1A1A2A,  // Very dark background
        120       // High transparency for glass effect
    );
    Compositor::render(fb);
}

// Example: Integrating with existing UI system
void demo_integrated_ui(limine_framebuffer* fb) {
    // Draw wallpaper first (background)
    draw_wallpaper(fb, 0x0D1117);
    
    // Create compositor windows on top
    int main_win = Compositor::create_window(
        100, 100,
        600, 400,
        "Main Application",
        0x2A2A3A,
        230
    );
    
    // Render compositor (transparent windows blend with wallpaper)
    Compositor::render(fb);
    
    // Draw UI elements on top (taskbar, etc.)
    draw_taskbar(fb);
    draw_top_bar(fb);
}
