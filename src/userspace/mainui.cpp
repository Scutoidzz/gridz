/*
 * mainui.cpp — Gridz OS desktop UI
 *
 * Draws:  wallpaper, top bar (RTC clock), taskbar (Gridz button),
 *         start menu, neofetch window, clock window.
 * Routes: mouse clicks to the correct UI element.
 */

#include "terminal.hpp"
#include "ui/app_manager.hpp"
#include "ui/components/comp.hpp"
#include "ui/compositor.hpp"
#include "ui/rtc.hpp"
#include "ui/ui.hpp"

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

/* Draw a window box with a title bar and red ✕ close button.
 * Returns the Y coordinate of the first usable pixel inside the window. */
int draw_window(limine_framebuffer *fb, int x, int y, int w, int h,
                const char *title, uint32_t title_color) {
  // Corner mask: number of pixels to skip from each side for the first 3 and
  // last 3 rows
  static const int skip[] = {3, 2, 1};

  for (int row = 0; row < h; row++) {
    int current_y = y + row;
    int current_x = x;
    int current_w = w;

    // Apply rounding at top and bottom
    if (row < 3) {
      current_x += skip[row];
      current_w -= skip[row] * 2;
    } else if (row >= h - 3) {
      current_x += skip[h - 1 - row];
      current_w -= skip[h - 1 - row] * 2;
    }

    uint32_t color = (row < 22) ? title_color : 0x1A1A2E;
    fil(fb, current_x, current_y, current_w, 1, color);
  }

  txt(fb, x + 6, y + 7, title, 0xFFFFFF);
  fil(fb, x + w - 20, y + 3, 16, 16, 0xCC2233); // close button
  txt(fb, x + w - 16, y + 7, "X", 0xFFFFFF);

  return y + 22;
}

/* Returns true if point (px,py) is inside the close button of a window
 * whose top-right corner is at (wx+ww, wy). */
bool hit_close(int px, int py, int wx, int wy, int ww) {
  return px >= wx + ww - 20 && px <= wx + ww - 4 && py >= wy + 3 &&
         py <= wy + 19;
}

/* ── CPUID: read CPU brand string into out[49] ───────────────────────────────
 */
static void get_cpu_brand(char *out) {
  uint32_t regs[4];
  __asm__ volatile("cpuid"
                   : "=a"(regs[0]), "=b"(regs[1]), "=c"(regs[2]), "=d"(regs[3])
                   : "a"(0x80000000u));

  if (regs[0] < 0x80000004u) {
    out[0] = '?';
    out[1] = '\0';
    return;
  }

  for (int leaf = 0; leaf < 3; leaf++) {
    __asm__ volatile("cpuid"
                     : "=a"(regs[0]), "=b"(regs[1]), "=c"(regs[2]),
                       "=d"(regs[3])
                     : "a"(0x80000002u + (uint32_t)leaf));
    for (int reg = 0; reg < 4; reg++) {
      uint32_t r = regs[reg];
      int base = leaf * 16 + reg * 4;
      out[base + 0] = (char)(r & 0xFF);
      out[base + 1] = (char)((r >> 8) & 0xFF);
      out[base + 2] = (char)((r >> 16) & 0xFF);
      out[base + 3] = (char)((r >> 24) & 0xFF);
    }
  }
  out[48] = '\0';

  // Trim leading spaces
  char *p = out;
  while (*p == ' ')
    p++;
  if (p != out) {
    int i = 0;
    while (p[i]) {
      out[i] = p[i];
      i++;
    }
    out[i] = '\0';
  }
}

/* ══════════════════════════════════════════════════════════════════════════════
 * SECTION 2 — UI STATE
 * ══════════════════════════════════════════════════════════════════════════════
 */

bool start_menu_open = false;
bool neofetch_open = false;
bool clock_open = false;
bool installer_open = false; // set properly by init_ui based on boot medium
bool installer_available = false; // true only when booted from installer ISO
int cursor_saved_x = -1, cursor_saved_y = -1;

/* Cached framebuffer dimensions (set by init_ui) */
int g_w = 0;
int g_h = 0;

/* Compositor window IDs */
static int neofetch_window_id = -1;
static int clock_window_id = -1;

/* Mouse drag state */
bool mouse_dragging = false;

// Drag handling functions
void handle_mouse_drag(int mx, int my) {
  if (mouse_dragging) {
    Compositor::update_drag(mx, my);
  }
}

void end_mouse_drag() {
  if (mouse_dragging) {
    Compositor::end_drag();
    mouse_dragging = false;
  }
}

/* Start-menu panel geometry */
static const int SM_W = 180;
static const int SM_ITEM = 30; // height of each menu row
static const int SM_HDR = 24;  // header height
static const int SM_CNT = 6;   // number of items

static int sm_x = 4;
static int sm_y = 0;
static int sm_h = SM_HDR + SM_CNT * SM_ITEM + 2;

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
  txt(fb, 8, 10, "Gridz OS", 0x7FCBFF);
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
static const MenuItem MENU_ITEMS[SM_CNT] = {
    {"Terminal", 0x1A3A1A}, {"Neofetch", 0x1A1A4A},  {"Doom", 0x4A1A1A},
    {"Clock", 0x1A3A3A},    {"Installer", 0x2A6EBB}, // only shown/functional
                                                     // when installer_available
    {"Halt", 0x3A1A1A},
};

void draw_start_menu(limine_framebuffer *fb) {
  if (!start_menu_open)
    return;

  // Panel border + background
  fil(fb, sm_x, sm_y, SM_W, sm_h, 0x111827);
  fil(fb, sm_x, sm_y, SM_W, 1, 0x3A8EFF);
  fil(fb, sm_x, sm_y + sm_h - 1, SM_W, 1, 0x3A8EFF);
  fil(fb, sm_x, sm_y, 1, sm_h, 0x3A8EFF);
  fil(fb, sm_x + SM_W - 1, sm_y, 1, sm_h, 0x3A8EFF);

  // Header row
  fil(fb, sm_x, sm_y, SM_W, 22, 0x0D1117);
  txt(fb, sm_x + 8, sm_y + 7, "Gridz OS", 0x7FCBFF);
  fil(fb, sm_x, sm_y + 22, SM_W, 1, 0x2A3A4A);

  // Menu items
  int iy = sm_y + SM_HDR;
  for (int i = 0; i < SM_CNT; i++) {
    // Installer item is greyed out when not booted from ISO
    bool grayed = (i == 4 && !installer_available);
    uint32_t bg = grayed ? 0x0D0D14 : 0x111827;
    uint32_t fg = grayed ? 0x555566 : 0xDDEEFF;
    uint32_t ic = grayed ? 0x222233 : MENU_ITEMS[i].icon_color;
    fil(fb, sm_x + 1, iy, SM_W - 2, SM_ITEM, bg);
    fil(fb, sm_x + 4, iy + 8, 16, 16, ic);
    txt(fb, sm_x + 26, iy + 11, MENU_ITEMS[i].label, fg);
    fil(fb, sm_x, iy + SM_ITEM - 1, SM_W, 1, 0x1A2A3A);
    iy += SM_ITEM;
  }
}

/* ══════════════════════════════════════════════════════════════════════════════
 * SECTION 5 — NEOFETCH WINDOW
 * ══════════════════════════════════════════════════════════════════════════════
 */

static uint32_t neofetch_buf[440 * (220 - MAX_TITLEBAR_HEIGHT)];

void draw_neofetch(limine_framebuffer *fb) {
  if (!neofetch_open) {
    if (neofetch_window_id >= 0) {
      Compositor::destroy_window(neofetch_window_id);
      neofetch_window_id = -1;
    }
    return;
  }

  int W = (int)fb->width;
  int H = (int)fb->height;
  int wx = W / 2 - 220, wy = H / 2 - 110;
  int ww = 440, wh = 220;

  // Create or update compositor window
  if (neofetch_window_id < 0) {
    neofetch_window_id = Compositor::create_window(
        wx, wy, ww, wh, "System Information", 0x1A3A5C, 240, 10);
    Compositor::windows[neofetch_window_id].content_buffer = neofetch_buf;
  }

  limine_framebuffer fake_fb;
  fake_fb.address = neofetch_buf;
  fake_fb.width = ww;
  fake_fb.height = wh - MAX_TITLEBAR_HEIGHT;
  fake_fb.pitch = ww * 4;

  // Clear background
  fil(&fake_fb, 0, 0, fake_fb.width, fake_fb.height, 0x1A3A5C);

  // Draw content inside window
  int iy = 8;

  // Gridz logo (colored stripes)
  static const uint32_t logo_colors[] = {0x3A8EFF, 0x2A6EBB, 0x1A4A88,
                                         0x0D2A55, 0x3A8EFF, 0x2A6EBB};
  for (int i = 0; i < 6; i++)
    fil(&fake_fb, 12, iy + i * 10, 40, 8, logo_colors[i]);

  // Info lines
  int tx = 60;
  int ty = iy;

  // OS
  txt(&fake_fb, tx, ty, "OS:   Gridz OS 0.1", 0x7FCBFF);
  ty += 14;

  // CPU brand from CPUID
  char cpu[49] = "";
  get_cpu_brand(cpu);
  char cpu_line[64] = "CPU:  ";
  int pos = 6;
  str_append(cpu_line, &pos, cpu, 62);
  txt(&fake_fb, tx, ty, cpu_line, 0xAADDFF);
  ty += 14;

  // Resolution
  char res_line[32] = "RES:  ";
  pos = 6;
  char tmp[12];
  u32_to_str((uint32_t)g_w, tmp);
  str_append(res_line, &pos, tmp, 30);
  str_append(res_line, &pos, "x", 30);
  u32_to_str((uint32_t)g_h, tmp);
  str_append(res_line, &pos, tmp, 30);
  txt(&fake_fb, tx, ty, res_line, 0xAADDFF);
  ty += 14;

  // Uptime
  uint32_t secs = timer_ticks / 1000;
  char up_line[48] = "UP:   ";
  pos = 6;
  u32_to_str(secs / 3600, tmp);
  str_append(up_line, &pos, tmp, 46);
  str_append(up_line, &pos, "h ", 46);
  u32_to_str((secs / 60) % 60, tmp);
  str_append(up_line, &pos, tmp, 46);
  str_append(up_line, &pos, "m ", 46);
  u32_to_str(secs % 60, tmp);
  str_append(up_line, &pos, tmp, 46);
  str_append(up_line, &pos, "s", 46);
  txt(&fake_fb, tx, ty, up_line, 0xAADDFF);
  ty += 14;

  txt(&fake_fb, tx, ty, "KERN: gridz-kernel-0.1", 0xAADDFF);
  ty += 14;
  txt(&fake_fb, tx, ty, "ARCH: x86_64", 0xAADDFF);
}

/* ══════════════════════════════════════════════════════════════════════════════
 * SECTION 6 — CLOCK WINDOW
 * ══════════════════════════════════════════════════════════════════════════════
 */

/* Draw a single 8×8 bitmap character at 5× scale */
static void draw_char_big(limine_framebuffer *fb, int x, int y, char c,
                          uint32_t color) {
  uint8_t bmap[8];
  if (!get_char_bitmap(c, bmap))
    return;

  const int SCALE = 5;
  for (int row = 0; row < 8; row++) {
    for (int col = 0; col < 8; col++) {
      if (bmap[row] & (1 << col))
        fil(fb, x + col * SCALE, y + row * SCALE, SCALE, SCALE, color);
    }
  }
}

/* Draw a string at 5× scale */
static void draw_str_big(limine_framebuffer *fb, int x, int y, const char *s,
                         uint32_t color) {
  while (*s) {
    draw_char_big(fb, x, y, *s++, color);
    x += 8 * 5 + 2; // 40px glyph + 2px gap
  }
}

static uint32_t clock_buf[340 * (140 - MAX_TITLEBAR_HEIGHT)];

void draw_clock_window(limine_framebuffer *fb) {
  if (!clock_open) {
    if (clock_window_id >= 0) {
      Compositor::destroy_window(clock_window_id);
      clock_window_id = -1;
    }
    return;
  }

  int W = (int)fb->width;
  int H = (int)fb->height;

  // Window: 340×140, centered
  const int ww = 340, wh = 140;
  int wx = W / 2 - ww / 2;
  int wy = H / 2 - wh / 2;

  // Create or update compositor window
  if (clock_window_id < 0) {
    clock_window_id =
        Compositor::create_window(wx, wy, ww, wh, "Clock", 0x0D2A3A, 240, 10);
    Compositor::windows[clock_window_id].content_buffer = clock_buf;
  }

  limine_framebuffer fake_fb;
  fake_fb.address = clock_buf;
  fake_fb.width = ww;
  fake_fb.height = wh - MAX_TITLEBAR_HEIGHT;
  fake_fb.pitch = ww * 4;

  fil(&fake_fb, 0, 0, fake_fb.width, fake_fb.height, 0x0D2A3A);

  // Large "HH:MM:SS" — 8 chars × (40 + 2) = 336 px wide
  char clk[9];
  rtc_format(rtc_now(), clk);
  int str_w = 8 * (8 * 5 + 2);
  draw_str_big(&fake_fb, (ww - str_w) / 2, 40 - MAX_TITLEBAR_HEIGHT, clk,
               0x7FCBFF);
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
    // Check if clicked on close button (top-right corner)
    if (Compositor::is_on_titlebar(clicked_window, mx, my)) {
      Window *win = &Compositor::windows[clicked_window];
      int close_x = win->x + win->width - 25;
      int close_y = win->y + 8;
      if (mx >= close_x - 8 && mx <= close_x + 8 && my >= close_y - 8 &&
          my <= close_y + 8) {
        // Close button clicked
        if (clicked_window == neofetch_window_id) {
          neofetch_open = false;
          neofetch_window_id = -1;
        } else if (clicked_window == clock_window_id) {
          clock_open = false;
          clock_window_id = -1;
        }
        Compositor::destroy_window(clicked_window);
        return true;
      }

      // Start dragging if on titlebar
      Compositor::start_drag(clicked_window, mx, my);
      Compositor::focus_window(clicked_window);
      mouse_dragging = true;
      return true;
    }

    // Clicked on window body - just focus it
    Compositor::focus_window(clicked_window);
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

      start_menu_open = false;

      if (idx >= 0 && idx < SM_CNT) {
        switch (idx) {
        case 0: // Terminal
          in_gui_mode = false;
          break;
        case 1: // Neofetch
          neofetch_open = true;
          break;
        case 2: // Doom
          launch_doom = true;
          break;
        case 3: // Clock
          clock_open = true;
          break;
        case 4: // Installer (only active when booted from ISO)
          if (installer_available)
            installer_open = true;
          break;
        case 5: // Shutdown
          __asm__ volatile("cli; hlt");
          break;
        }
      }
    } else {
      // Clicked outside the menu — just close it
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
