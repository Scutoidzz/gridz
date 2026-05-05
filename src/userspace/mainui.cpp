
#include "../ui/app_manager.hpp"
#include "../ui/components/comp.hpp"
#include "../ui/compositor.hpp"
#include "../ui/rtc.hpp"
#include "../ui/ui.hpp"
#include "terminal.hpp"
#include "font.hpp"

#include <stddef.h>
#include <stdint.h>

extern "C" void serial_print(const char *s);
extern uint64_t hhdm_offset;

/* ── Externs defined elsewhere ───────────────────────────────────────────────
 */
extern Terminal *global_term;
extern volatile int mouse_x;
extern volatile int mouse_y;
extern volatile uint8_t mouse_buttons;
extern volatile uint32_t timer_ticks;
extern bool in_gui_mode;
extern bool launch_doom;
extern void draw_installer(limine_framebuffer *fb);
extern bool handle_installer_click(limine_framebuffer *fb, int mx, int my);
extern "C" volatile struct limine_module_response *get_module_response();

/* Returns true only when booting from the installer ISO (the bootblock
 * module is present). When booting from an installed drive it is absent. */
static bool is_installer_boot() {
  auto res = get_module_response();
  if (!res)
    return false;
  for (uint64_t i = 0; i < res->module_count; i++) {
    const char *cmd = res->modules[i]->cmdline;
    /* strcmp-lite: check for "install_bootblock" */
    const char *ref = "install_bootblock";
    const char *a = cmd;
    const char *b = ref;
    while (*a && *b && *a == *b) {
      a++;
      b++;
    }
    if (*a == '\0' && *b == '\0')
      return true;
  }
  return false;
}

static void fil(limine_framebuffer *fb, int x, int y, int w, int h,
                uint32_t color) {
  draw_rectangle(fb, x, y, w, h, color);
}

static void txt(limine_framebuffer *fb, int x, int y, const char *s,
                uint32_t color) {
  draw_string(fb, x, y, s, color);
}

/* uint32 → decimal string (no stdlib) */
static void u32_to_str(uint32_t v, char *buf) {
  if (!v) {
    buf[0] = '0';
    buf[1] = '\0';
    return;
  }
  char tmp[12];
  int len = 0;
  while (v) {
    tmp[len++] = '0' + v % 10;
    v /= 10;
  }
  for (int i = 0; i < len; i++)
    buf[i] = tmp[len - 1 - i];
  buf[len] = '\0';
}

/* Append src to dst starting at index *pos; update *pos */
static void str_append(char *dst, int *pos, const char *src, int max) {
  while (*src && *pos < max)
    dst[(*pos)++] = *src++;
  dst[*pos] = '\0';
}

static int str_len(const char *s) {
  int len = 0;
  while (s[len]) len++;
  return len;
}

/* Draw a window box with a title bar and red ✕ close button.
 * Returns the Y coordinate of the first usable pixel inside the window. */
int draw_window(limine_framebuffer *fb, int x, int y, int w, int h,
                const char *title) {
  static const int skip[] = {3, 2, 1};

  for (int row = 0; row < h; row++) {
    int current_y = y + row;
    int current_x = x;
    int current_w = w;

    if (row < 3) {
      current_x += skip[row];
      current_w -= skip[row] * 2;
    } else if (row >= h - 3) {
      current_x += skip[h - 1 - row];
      current_w -= skip[h - 1 - row] * 2;
    }

    uint32_t color = (row < 22) ? 0x3A6DB5 : 0x1A1A2E;
    fil(fb, current_x, current_y, current_w, 1, color);
  }

  txt(fb, x + 6, y + 7, title, 0xFFFFFF);
  fil(fb, x + w - 20, y + 3, 16, 16, 0xCC2233);
  txt(fb, x + w - 16, y + 7, "X", 0xFFFFFF);

  return y + 22;
}

bool hit_close(int px, int py, int wx, int wy, int ww) {
  return px >= wx + ww - 20 && px <= wx + ww - 4 && py >= wy + 3 &&
         py <= wy + 19;
}

/* ── CPUID: read CPU brand string into out[49] ───────────────────────────────
 */
/* ══════════════════════════════════════════════════════════════════════════════
 * SECTION 2 — UI STATE
 * ══════════════════════════════════════════════════════════════════════════════
 */

bool start_menu_open = false;
bool neofetch_open = false;
bool clock_open = false;
bool calculator_open = false;
bool filemanager_open = false;
bool installer_open = false; // set properly by init_ui based on boot medium
bool installer_available = false; // true only when booted from installer ISO
int cursor_saved_x = -1, cursor_saved_y = -1;

/* Cached framebuffer dimensions (set by init_ui) */
int g_w = 0;
int g_h = 0;

/* Compositor window IDs */
int neofetch_window_id = -1;
int clock_window_id = -1;
int calc_window_id = -1;
int fm_window_id = -1;

#include "apps/neofetch.cpp"
#include "apps/clock.cpp"
#include "apps/calculator.cpp"
#include "apps/filemanager.cpp"

/* App registry for dynamic window management */
struct AppEntry {
  const char* name;
  int* window_id;
  bool* open_flag;
};

static const AppEntry app_registry[] = {
  {"Neofetch", &neofetch_window_id, &neofetch_open},
  {"Clock", &clock_window_id, &clock_open},
  {"Calculator", &calc_window_id, &calculator_open},
  {"Files", &fm_window_id, &filemanager_open},
};
static const int app_registry_size = sizeof(app_registry) / sizeof(app_registry[0]);

/* Mouse drag/resize state */
bool mouse_dragging = false;
bool mouse_resizing = false;

void handle_mouse_drag(int mx, int my) {
  if (mouse_dragging)
    Compositor::update_drag(mx, my);
}

void end_mouse_drag() {
  if (mouse_dragging) {
    Compositor::end_drag();
    mouse_dragging = false;
  }
}

void handle_mouse_resize(int mx, int my) {
  if (mouse_resizing)
    Compositor::update_resize(mx, my);
}

void end_mouse_resize() {
  if (mouse_resizing) {
    Compositor::end_resize();
    mouse_resizing = false;
  }
}

/* Start-menu panel geometry */
static const int SM_W    = 180;
static const int SM_ITEM = 30; // height of each menu row
static const int SM_HDR  = 24; // header height

static const int SM_BASE = 7;

static int sm_x = 4;
static int sm_y = 0;
static int sm_h = SM_HDR + SM_BASE * SM_ITEM + 2; // recalculated after scan

/* ── Cursor save/restore buffer ───────────────────────────────────────────────
 */
static uint32_t cursor_bg[12 * 19];

static void cursor_save(limine_framebuffer *fb, int cx, int cy) {
  uint32_t *px = (uint32_t *)((uint64_t)fb->address);
  int stride = (int)(fb->pitch / 4);
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

static void cursor_restore(limine_framebuffer *fb) {
  if (cursor_saved_x < 0)
    return;
  uint32_t *px = (uint32_t *)((uint64_t)fb->address);
  int stride = (int)(fb->pitch / 4);
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

void ui_cursor_invalidate() {
  cursor_saved_x = -1;
  cursor_saved_y = -1;
}

void draw_wallpaper_gradient(limine_framebuffer *fb) {
  uint32_t *px = (uint32_t *)((uint64_t)fb->address);
  int stride = (int)(fb->pitch / 4);
  int W = (int)fb->width;
  int H = (int)fb->height;

  for (int y = 28; y < H - 38; y++) {
    // Subtle top-to-bottom gradient: dark navy → slightly darker
    uint8_t t = (uint8_t)((y - 28) * 8 / (H - 66));
    uint32_t color = ((uint32_t)(0x1A - t / 4) << 16) |
                     ((uint32_t)(0x26 - t / 4) << 8) | (uint32_t)(0x36 - t / 4);
    for (int x = 0; x < W; x++)
      px[y * stride + x] = color;
  }
}

void init_ui(limine_framebuffer *fb) {
  g_w = (int)fb->width;
  g_h = (int)fb->height;

  // Recalculate start-menu position (sits just above the taskbar)
  sm_y = g_h - 38 - sm_h - 2;

  // Detect boot medium: show installer only when running from the ISO
  installer_available = is_installer_boot();

  // Reset window state
  start_menu_open = false;
  neofetch_open = false;
  clock_open = false;
  installer_open = installer_available;
  cursor_saved_x = -1;

  // Initialize compositor for transparent windows
  Compositor::init();

  // Draw initial desktop
  fil(fb, 0, 0, g_w, g_h, 0x0D1117);
  draw_wallpaper_gradient(fb);
  AppManager::init();
  draw_top_bar(fb);
  draw_taskbar(fb);
}

void draw_top_bar(limine_framebuffer *fb) {
  int W = (int)fb->width;

  fil(fb, 0, 0, W, 28, 0x0D1117);
  txt(fb, 8, 10, "Gridz", 0x7FCBFF);
  fil(fb, W - 4, 0, 4, 28, 0x3A8EFF); // right accent stripe
  fil(fb, 0, 27, W, 1, 0x3A8EFF);     // bottom border

  // Live RTC clock on the right
  char clk[9];
  rtc_format(rtc_now(), clk);
  txt(fb, W - 72, 10, clk, 0xBBDDFF);
}

void draw_taskbar(limine_framebuffer *fb) {
  int W = (int)fb->width;
  int H = (int)fb->height;
  int by = H - 38;

  fil(fb, 0, by, W, 38, 0x0D1117);
  fil(fb, 0, by, W, 1, 0x3A8EFF); // top border

  // "Gridz" start button — highlighted when menu is open
  uint32_t btn_bg = start_menu_open ? 0x2A5A8C : 0x1A3A5C;
  fil(fb, 6, by + 5, 60, 26, btn_bg);
  fil(fb, 6, by + 5, 60, 1, 0x3A8EFF); // top highlight
  txt(fb, 14, by + 14, "Gridz", 0xADE0FF);

  // Clock in system tray (right side)
  char clk[9];
  rtc_format(rtc_now(), clk);
  txt(fb, W - 72, by + 14, clk, 0x7FCBFF);
}

struct MenuItem {
  const char *label;
  uint32_t icon_color;
};
// Fixed items — Halt is always last and drawn separately
static const MenuItem MENU_ITEMS[] = {
    {"Terminal",    0x1A3A1A},
    {"Neofetch",    0x1A1A4A},
    {"Doom",        0x4A1A1A},
    {"Clock",       0x1A3A3A},
    {"Calculator",  0x1A3A2A},
    {"Files",       0x1A2A3A},
};
static const int MENU_FIXED = sizeof(MENU_ITEMS) / sizeof(MENU_ITEMS[0]);

static void draw_menu_row(limine_framebuffer *fb, int iy,
                          const char *label, uint32_t icon_color) {
    fil(fb, sm_x + 1, iy, SM_W - 2, SM_ITEM, 0x111827);
    fil(fb, sm_x + 4, iy + 8, 16, 16, icon_color);
    txt(fb, sm_x + 26, iy + 11, label, 0xDDEEFF);
    fil(fb, sm_x, iy + SM_ITEM - 1, SM_W, 1, 0x1A2A3A);
}

void draw_start_menu(limine_framebuffer *fb) {
  if (!start_menu_open)
    return;

  int total = MENU_FIXED + 1; // +1 for Halt
  sm_h = SM_HDR + total * SM_ITEM + 2;
  sm_y = (int)fb->height - 38 - sm_h - 2;

  // Panel border + background
  fil(fb, sm_x, sm_y, SM_W, sm_h, 0x111827);
  fil(fb, sm_x, sm_y, SM_W, 1, 0x3A8EFF);
  fil(fb, sm_x, sm_y + sm_h - 1, SM_W, 1, 0x3A8EFF);
  fil(fb, sm_x, sm_y, 1, sm_h, 0x3A8EFF);
  fil(fb, sm_x + SM_W - 1, sm_y, 1, sm_h, 0x3A8EFF);

  // Header row
  fil(fb, sm_x, sm_y, SM_W, 22, 0x0D1117);
  txt(fb, sm_x + 8, sm_y + 7, "Gridz", 0x7FCBFF);
  fil(fb, sm_x, sm_y + 22, SM_W, 1, 0x2A3A4A);

  int iy = sm_y + SM_HDR;

  for (int i = 0; i < MENU_FIXED; i++) {
    draw_menu_row(fb, iy, MENU_ITEMS[i].label, MENU_ITEMS[i].icon_color);
    iy += SM_ITEM;
  }
  draw_menu_row(fb, iy, "Halt", 0x3A1A1A);
}

/* ══════════════════════════════════════════════════════════════════════════════
 * SECTION 7 — CLICK ROUTING
 * ══════════════════════════════════════════════════════════════════════════════
 */

/*
 * Returns true if click was handled (caller should redraw cursor on top).
 * All window-close and menu-open logic is here.
 */
bool handle_ui_click(limine_framebuffer *fb, int mx, int my) {
  int W = (int)fb->width;
  int H = (int)fb->height;
  int by = H - 38; // taskbar top y

  /* ── Close open windows first ─────────────────────────────────────────── */
  if (installer_open) {
    if (handle_installer_click(fb, mx, my)) {
      return true;
    }
    return true;
  }

  /* ── Compositor window handling (dragging, focusing, closing) ─────────── */
  int clicked_window = Compositor::get_window_at(mx, my);

  if (clicked_window >= 0) {
    if (Compositor::is_on_titlebar(clicked_window, mx, my)) {
      Window *win = &Compositor::windows[clicked_window];
      int close_x = win->x + win->width - 25;
      int close_y = win->y + 8;
      if (mx >= close_x - 8 && mx <= close_x + 8 && my >= close_y - 8 &&
          my <= close_y + 8) {
        for (int i = 0; i < app_registry_size; i++) {
          if (clicked_window == *app_registry[i].window_id) {
            *app_registry[i].open_flag = false;
            *app_registry[i].window_id = -1;
            Compositor::destroy_window(clicked_window);
            return true;
          }
        }
        return true;
      }

      Compositor::start_drag(clicked_window, mx, my);
      Compositor::focus_window(clicked_window);
      mouse_dragging = true;
      return true;
    }

    if (Compositor::is_on_resize_handle(clicked_window, mx, my)) {
      Compositor::start_resize(clicked_window, mx, my);
      Compositor::focus_window(clicked_window);
      mouse_resizing = true;
      return true;
    }

    // Window body click
    Compositor::focus_window(clicked_window);
    if (clicked_window == calc_window_id)
      handle_calculator_click(clicked_window, mx, my);
    else if (clicked_window == fm_window_id)
      handle_filemanager_click(clicked_window, mx, my);
    return true;
  }

  /* ── Start button (6,by+5) 60×26 ─────────────────────────────────────── */
  bool hit_start = (mx >= 6 && mx <= 66 && my >= by + 5 && my <= by + 31);
  if (hit_start) {
    start_menu_open = !start_menu_open;
    return true;
  }

  /* ── Start menu items ─────────────────────────────────────────────────── */
  if (start_menu_open) {
    bool in_menu =
        (mx >= sm_x && mx <= sm_x + SM_W && my >= sm_y && my <= sm_y + sm_h);
    if (in_menu) {
      int rel = my - (sm_y + SM_HDR);
      int idx = (rel >= 0) ? rel / SM_ITEM : -1;
      int total = MENU_FIXED + 1;

      start_menu_open = false;

      if (idx >= 0 && idx < total) {
        if (idx < MENU_FIXED) {
          switch (idx) {
          case 0: in_gui_mode = false;    break;
          case 1: neofetch_open = true;   break;
          case 2: launch_doom = true;     break;
          case 3: clock_open = true;      break;
          case 4: calculator_open = true; break;
          case 5: filemanager_open = true;break;
          }
        } else {
          // Halt
          __asm__ volatile("cli; hlt");
        }
      }
    } else {
      start_menu_open = false;
    }
    return true;
  }

  return false;
}

/* ══════════════════════════════════════════════════════════════════════════════
 * SECTION 8 — SOFTWARE CURSOR
 * ══════════════════════════════════════════════════════════════════════════════
 */

void draw_ui_cursor(limine_framebuffer *fb) {
  draw_cursor(fb, mouse_x, mouse_y);
}
