/*
 * mainui.cpp — Gridz OS desktop UI
 *
 * Draws:  wallpaper, top bar (RTC clock), taskbar (Gridz button),
 *         start menu, neofetch window, clock window.
 * Routes: mouse clicks to the correct UI element.
 */

#include "../ui/ui.hpp"
#include "../ui/components/comp.hpp"
#include "../ui/app_manager.hpp"
#include "../ui/rtc.hpp"
#include "../terminal.hpp"

/* ── Externs defined elsewhere ─────────────────────────────────────────────── */
extern Terminal*         global_term;
extern volatile int      mouse_x;
extern volatile int      mouse_y;
extern volatile uint8_t  mouse_buttons;
extern volatile uint32_t timer_ticks;
extern bool              in_gui_mode;
extern bool              launch_doom;

static void fil(limine_framebuffer* fb,
                int x, int y, int w, int h, uint32_t color) {
    draw_rectangle(fb, x, y, w, h, color);
}

static void txt(limine_framebuffer* fb,
                int x, int y, const char* s, uint32_t color) {
    draw_string(fb, x, y, s, color);
}

/* uint32 → decimal string (no stdlib) */
static void u32_to_str(uint32_t v, char* buf) {
    if (!v) { buf[0] = '0'; buf[1] = '\0'; return; }
    char tmp[12];
    int  len = 0;
    while (v) { tmp[len++] = '0' + v % 10; v /= 10; }
    for (int i = 0; i < len; i++) buf[i] = tmp[len - 1 - i];
    buf[len] = '\0';
}

/* Append src to dst starting at index *pos; update *pos */
static void str_append(char* dst, int* pos, const char* src, int max) {
    while (*src && *pos < max) dst[(*pos)++] = *src++;
    dst[*pos] = '\0';
}

/* Draw a window box with a title bar and red ✕ close button.
 * Returns the Y coordinate of the first usable pixel inside the window. */
static int draw_window(limine_framebuffer* fb,
                       int x, int y, int w, int h,
                       const char* title, uint32_t title_color) {
    fil(fb, x,         y,          w,  h, 0x1A1A2E);    // body
    fil(fb, x,         y,          w, 22, title_color); // title bar
    txt(fb, x + 6,     y + 7,      title, 0xFFFFFF);
    fil(fb, x + w - 20, y + 3,     16, 16, 0xCC2233);   // close button
    txt(fb, x + w - 16, y + 7,    "X", 0xFFFFFF);
    // border
    fil(fb, x,          y,          w,  1, 0x3A8EFF);
    fil(fb, x,          y + h - 1,  w,  1, 0x3A8EFF);
    fil(fb, x,          y,          1,  h, 0x3A8EFF);
    fil(fb, x + w - 1,  y,          1,  h, 0x3A8EFF);
    return y + 22;
}

/* Returns true if point (px,py) is inside the close button of a window
 * whose top-right corner is at (wx+ww, wy). */
static bool hit_close(int px, int py, int wx, int wy, int ww) {
    return px >= wx + ww - 20 && px <= wx + ww - 4
        && py >= wy + 3       && py <= wy + 19;
}

/* ── CPUID: read CPU brand string into out[49] ─────────────────────────────── */
static void get_cpu_brand(char* out) {
    uint32_t regs[4];
    __asm__ volatile("cpuid"
        : "=a"(regs[0]), "=b"(regs[1]), "=c"(regs[2]), "=d"(regs[3])
        : "a"(0x80000000u));

    if (regs[0] < 0x80000004u) { out[0] = '?'; out[1] = '\0'; return; }

    for (int leaf = 0; leaf < 3; leaf++) {
        __asm__ volatile("cpuid"
            : "=a"(regs[0]), "=b"(regs[1]), "=c"(regs[2]), "=d"(regs[3])
            : "a"(0x80000002u + (uint32_t)leaf));
        for (int reg = 0; reg < 4; reg++) {
            uint32_t r = regs[reg];
            int base = leaf * 16 + reg * 4;
            out[base + 0] = (char)( r        & 0xFF);
            out[base + 1] = (char)((r >>  8) & 0xFF);
            out[base + 2] = (char)((r >> 16) & 0xFF);
            out[base + 3] = (char)((r >> 24) & 0xFF);
        }
    }
    out[48] = '\0';

    // Trim leading spaces
    char* p = out;
    while (*p == ' ') p++;
    if (p != out) {
        int i = 0;
        while (p[i]) { out[i] = p[i]; i++; }
        out[i] = '\0';
    }
}

/* ══════════════════════════════════════════════════════════════════════════════
 * SECTION 2 — UI STATE
 * ══════════════════════════════════════════════════════════════════════════════ */

static bool start_menu_open = false;
static bool neofetch_open   = false;
static bool clock_open      = false;

/* Cached framebuffer dimensions (set by init_ui) */
static int g_w = 0;
static int g_h = 0;

/* Start-menu panel geometry */
static const int SM_W    = 180;
static const int SM_ITEM = 30;   // height of each menu row
static const int SM_HDR  = 24;   // header height
static const int SM_CNT  = 5;    // number of items

static int sm_x = 4;
static int sm_y = 0;
static int sm_h = SM_HDR + SM_CNT * SM_ITEM + 2;

/* ── Cursor save/restore buffer ─────────────────────────────────────────────── */
static uint32_t cursor_bg[12 * 19];
static int      cursor_saved_x = -1;
static int      cursor_saved_y = -1;

static void cursor_save(limine_framebuffer* fb, int cx, int cy) {
    uint32_t* px = (uint32_t*)fb->address;
    int stride   = (int)(fb->pitch / 4);
    int W = (int)fb->width;
    int H = (int)fb->height;

    for (int row = 0; row < 19; row++) {
        for (int col = 0; col < 12; col++) {
            int sx = cx + col, sy = cy + row;
            cursor_bg[row * 12 + col] =
                (sx >= 0 && sx < W && sy >= 0 && sy < H) ? px[sy * stride + sx] : 0;
        }
    }
    cursor_saved_x = cx;
    cursor_saved_y = cy;
}

static void cursor_restore(limine_framebuffer* fb) {
    if (cursor_saved_x < 0) return;
    uint32_t* px = (uint32_t*)fb->address;
    int stride   = (int)(fb->pitch / 4);
    int W = (int)fb->width;
    int H = (int)fb->height;

    for (int row = 0; row < 19; row++) {
        for (int col = 0; col < 12; col++) {
            int sx = cursor_saved_x + col, sy = cursor_saved_y + row;
            if (sx >= 0 && sx < W && sy >= 0 && sy < H)
                px[sy * stride + sx] = cursor_bg[row * 12 + col];
        }
    }
}

/* ══════════════════════════════════════════════════════════════════════════════
 * SECTION 3 — WALLPAPER, TOP BAR, TASKBAR
 * ══════════════════════════════════════════════════════════════════════════════ */

static void draw_wallpaper(limine_framebuffer* fb) {
    uint32_t* px = (uint32_t*)fb->address;
    int stride   = (int)(fb->pitch / 4);
    int W = (int)fb->width;
    int H = (int)fb->height;

    for (int y = 28; y < H - 38; y++) {
        // Subtle top-to-bottom gradient: dark navy → slightly darker
        uint8_t t = (uint8_t)((y - 28) * 8 / (H - 66));
        uint32_t color = ((uint32_t)(0x1A - t / 4) << 16)
                       | ((uint32_t)(0x26 - t / 4) <<  8)
                       |  (uint32_t)(0x36 - t / 4);
        for (int x = 0; x < W; x++) px[y * stride + x] = color;
    }
}

void init_ui(limine_framebuffer* fb) {
    g_w = (int)fb->width;
    g_h = (int)fb->height;

    // Recalculate start-menu position (sits just above the taskbar)
    sm_y = g_h - 38 - sm_h - 2;

    // Reset window state
    start_menu_open = false;
    neofetch_open   = false;
    clock_open      = false;
    cursor_saved_x  = -1;

    // Draw initial desktop
    fil(fb, 0, 0, g_w, g_h, 0x0D1117);
    draw_wallpaper(fb);
    AppManager::init();
    draw_top_bar(fb);
    draw_taskbar(fb);
}

void draw_top_bar(limine_framebuffer* fb) {
    int W = (int)fb->width;

    fil(fb, 0,     0, W, 28, 0x0D1117);
    txt(fb, 8,    10, "Gridz OS", 0x7FCBFF);
    fil(fb, W - 4, 0, 4, 28, 0x3A8EFF);  // right accent stripe
    fil(fb, 0,    27, W,  1, 0x3A8EFF);  // bottom border

    // Live RTC clock on the right
    char clk[9];
    rtc_format(rtc_now(), clk);
    txt(fb, W - 72, 10, clk, 0xBBDDFF);
}

void draw_taskbar(limine_framebuffer* fb) {
    int W    = (int)fb->width;
    int H    = (int)fb->height;
    int by   = H - 38;

    fil(fb, 0, by, W, 38, 0x0D1117);
    fil(fb, 0, by, W,  1, 0x3A8EFF);   // top border

    // "Gridz" start button — highlighted when menu is open
    uint32_t btn_bg = start_menu_open ? 0x2A5A8C : 0x1A3A5C;
    fil(fb, 6, by + 5, 60, 26, btn_bg);
    fil(fb, 6, by + 5, 60,  1, 0x3A8EFF);  // top highlight
    txt(fb, 14, by + 14, "Gridz", 0xADE0FF);

    // Clock in system tray (right side)
    char clk[9];
    rtc_format(rtc_now(), clk);
    txt(fb, W - 72, by + 14, clk, 0x7FCBFF);
}

/* ══════════════════════════════════════════════════════════════════════════════
 * SECTION 4 — START MENU
 * ══════════════════════════════════════════════════════════════════════════════ */

struct MenuItem { const char* label; uint32_t icon_color; };
static const MenuItem MENU_ITEMS[SM_CNT] = {
    { "Terminal",  0x1A3A1A },
    { "Neofetch",  0x1A1A4A },
    { "Doom",      0x4A1A1A },
    { "Clock",     0x1A3A3A },
    { "Shutdown",  0x3A1A1A },
};

void draw_start_menu(limine_framebuffer* fb) {
    if (!start_menu_open) return;

    // Panel border + background
    fil(fb, sm_x,            sm_y,             SM_W, sm_h, 0x111827);
    fil(fb, sm_x,            sm_y,             SM_W,    1, 0x3A8EFF);
    fil(fb, sm_x,            sm_y + sm_h - 1,  SM_W,    1, 0x3A8EFF);
    fil(fb, sm_x,            sm_y,             1,    sm_h, 0x3A8EFF);
    fil(fb, sm_x + SM_W - 1, sm_y,             1,    sm_h, 0x3A8EFF);

    // Header row
    fil(fb, sm_x,     sm_y,      SM_W, 22, 0x0D1117);
    txt(fb, sm_x + 8, sm_y + 7,  "Gridz OS", 0x7FCBFF);
    fil(fb, sm_x,     sm_y + 22, SM_W,  1,  0x2A3A4A);

    // Menu items
    int iy = sm_y + SM_HDR;
    for (int i = 0; i < SM_CNT; i++) {
        fil(fb, sm_x + 1,  iy,              SM_W - 2, SM_ITEM, 0x111827);
        fil(fb, sm_x + 4,  iy + 8,          16, 16,   MENU_ITEMS[i].icon_color);
        txt(fb, sm_x + 26, iy + 11,         MENU_ITEMS[i].label, 0xDDEEFF);
        fil(fb, sm_x,      iy + SM_ITEM - 1, SM_W, 1, 0x1A2A3A);
        iy += SM_ITEM;
    }
}

/* ══════════════════════════════════════════════════════════════════════════════
 * SECTION 5 — NEOFETCH WINDOW
 * ══════════════════════════════════════════════════════════════════════════════ */

void draw_neofetch(limine_framebuffer* fb) {
    if (!neofetch_open) return;

    int W = (int)fb->width;
    int H = (int)fb->height;
    int wx = W / 2 - 220, wy = H / 2 - 110;
    int ww = 440,         wh = 220;
    int iy = draw_window(fb, wx, wy, ww, wh, "System Information", 0x1A3A5C) + 8;

    // Gridz logo (colored stripes)
    static const uint32_t logo_colors[] = {
        0x3A8EFF, 0x2A6EBB, 0x1A4A88, 0x0D2A55, 0x3A8EFF, 0x2A6EBB
    };
    for (int i = 0; i < 6; i++)
        fil(fb, wx + 12, iy + i * 10, 40, 8, logo_colors[i]);

    // Info lines
    int tx = wx + 60;
    int ty = iy;

    // OS
    txt(fb, tx, ty, "OS:   Gridz OS 0.1", 0x7FCBFF);
    ty += 14;

    // CPU brand from CPUID
    char cpu[49] = "";
    get_cpu_brand(cpu);
    char cpu_line[64] = "CPU:  ";
    int  pos = 6;
    str_append(cpu_line, &pos, cpu, 62);
    txt(fb, tx, ty, cpu_line, 0xAADDFF);
    ty += 14;

    // Resolution
    char res_line[32] = "RES:  ";
    pos = 6;
    char tmp[12];
    u32_to_str((uint32_t)g_w, tmp); str_append(res_line, &pos, tmp, 30);
    str_append(res_line, &pos, "x", 30);
    u32_to_str((uint32_t)g_h, tmp); str_append(res_line, &pos, tmp, 30);
    txt(fb, tx, ty, res_line, 0xAADDFF);
    ty += 14;

    // Uptime
    uint32_t secs = timer_ticks / 1000;
    char up_line[48] = "UP:   ";
    pos = 6;
    u32_to_str(secs / 3600,      tmp); str_append(up_line, &pos, tmp, 46);
    str_append(up_line, &pos, "h ", 46);
    u32_to_str((secs / 60) % 60, tmp); str_append(up_line, &pos, tmp, 46);
    str_append(up_line, &pos, "m ", 46);
    u32_to_str(secs % 60,        tmp); str_append(up_line, &pos, tmp, 46);
    str_append(up_line, &pos, "s",  46);
    txt(fb, tx, ty, up_line, 0xAADDFF);
    ty += 14;

    txt(fb, tx, ty, "KERN: gridz-kernel-0.1", 0xAADDFF); ty += 14;
    txt(fb, tx, ty, "ARCH: x86_64",           0xAADDFF);
}

/* ══════════════════════════════════════════════════════════════════════════════
 * SECTION 6 — CLOCK WINDOW
 * ══════════════════════════════════════════════════════════════════════════════ */

/* Draw a single 8×8 bitmap character at 5× scale */
static void draw_char_big(limine_framebuffer* fb, int x, int y, char c, uint32_t color) {
    uint8_t bmap[8];
    if (!get_char_bitmap(c, bmap)) return;

    const int SCALE = 5;
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            if (bmap[row] & (1 << col))
                fil(fb, x + col * SCALE, y + row * SCALE, SCALE, SCALE, color);
        }
    }
}

/* Draw a string at 5× scale */
static void draw_str_big(limine_framebuffer* fb, int x, int y,
                         const char* s, uint32_t color) {
    while (*s) {
        draw_char_big(fb, x, y, *s++, color);
        x += 8 * 5 + 2;   // 40px glyph + 2px gap
    }
}

void draw_clock_window(limine_framebuffer* fb) {
    if (!clock_open) return;

    int W = (int)fb->width;
    int H = (int)fb->height;

    // Window: 340×140, centered
    const int ww = 340, wh = 140;
    int wx = W / 2 - ww / 2;
    int wy = H / 2 - wh / 2;
    draw_window(fb, wx, wy, ww, wh, "Clock", 0x0D2A3A);

    // Large "HH:MM:SS" — 8 chars × (40 + 2) = 336 px wide
    char clk[9];
    rtc_format(rtc_now(), clk);
    int str_w = 8 * (8 * 5 + 2);
    draw_str_big(fb, wx + (ww - str_w) / 2, wy + 40, clk, 0x7FCBFF);
}

/* ══════════════════════════════════════════════════════════════════════════════
 * SECTION 7 — CLICK ROUTING
 * ══════════════════════════════════════════════════════════════════════════════ */

/*
 * Returns true if click was handled (caller should redraw cursor on top).
 * All window-close and menu-open logic is here.
 */
bool handle_ui_click(limine_framebuffer* fb, int mx, int my) {
    int W    = (int)fb->width;
    int H    = (int)fb->height;
    int by   = H - 38;   // taskbar top y

    /* ── Close open windows first ─────────────────────────────────────────── */
    if (neofetch_open) {
        int wx = W / 2 - 220, wy = H / 2 - 110;
        if (hit_close(mx, my, wx, wy, 440)) {
            neofetch_open = false;
            draw_wallpaper(fb);
            draw_top_bar(fb);
            draw_taskbar(fb);
            return true;
        }
        return true;   // absorb all clicks while window is open
    }

    if (clock_open) {
        int wx = W / 2 - 170, wy = H / 2 - 70;
        if (hit_close(mx, my, wx, wy, 340)) {
            clock_open = false;
            draw_wallpaper(fb);
            draw_top_bar(fb);
            draw_taskbar(fb);
        }
        return true;   // absorb all clicks while window is open
    }

    /* ── Start button (6,by+5) 60×26 ─────────────────────────────────────── */
    bool hit_start = (mx >= 6 && mx <= 66 && my >= by + 5 && my <= by + 31);
    if (hit_start) {
        start_menu_open = !start_menu_open;
        draw_taskbar(fb);
        if (start_menu_open) {
            draw_start_menu(fb);
        } else {
            draw_wallpaper(fb);
            draw_top_bar(fb);
            draw_taskbar(fb);
        }
        return true;
    }

    /* ── Start menu items ─────────────────────────────────────────────────── */
    if (start_menu_open) {
        bool in_menu = (mx >= sm_x && mx <= sm_x + SM_W
                     && my >= sm_y && my <= sm_y + sm_h);
        if (in_menu) {
            int rel = my - (sm_y + SM_HDR);
            int idx = (rel >= 0) ? rel / SM_ITEM : -1;

            start_menu_open = false;
            draw_wallpaper(fb);
            draw_top_bar(fb);
            draw_taskbar(fb);

            if (idx >= 0 && idx < SM_CNT) {
                switch (idx) {
                    case 0:  // Terminal
                        in_gui_mode = false;
                        break;
                    case 1:  // Neofetch
                        neofetch_open = true;
                        draw_neofetch(fb);
                        break;
                    case 2:  // Doom
                        launch_doom = true;
                        break;
                    case 3:  // Clock
                        clock_open = true;
                        draw_clock_window(fb);
                        break;
                    case 4:  // Shutdown
                        __asm__ volatile("cli; hlt");
                        break;
                }
            }
        } else {
            // Clicked outside the menu — just close it
            start_menu_open = false;
            draw_wallpaper(fb);
            draw_top_bar(fb);
            draw_taskbar(fb);
        }
        return true;
    }

    return false;
}

/* ══════════════════════════════════════════════════════════════════════════════
 * SECTION 8 — SOFTWARE CURSOR
 * ══════════════════════════════════════════════════════════════════════════════ */

void draw_ui_cursor(limine_framebuffer* fb) {
    cursor_restore(fb);
    cursor_save(fb, mouse_x, mouse_y);
    draw_cursor(fb, mouse_x, mouse_y);
}
