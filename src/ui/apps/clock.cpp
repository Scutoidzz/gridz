

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
