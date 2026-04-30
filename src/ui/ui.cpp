#include "ui.hpp"
#include "font.hpp"

extern uint64_t hhdm_offset;

void draw_rect(limine_framebuffer *fb, int x, int y, int width, int height,
               uint32_t color) {
  uint32_t *fb_ptr = (uint32_t *)((uint64_t)fb->address);
  int stride = fb->pitch / 4;

  for (int row = 0; row < height; row++) {
    for (int col = 0; col < width; col++) {
      fb_ptr[(y + row) * stride + (x + col)] = color;
    }
  }
}

void draw_launcher(limine_framebuffer *fb) {
  draw_rect(fb, 100, 100, 600, 400, 0x222222);
  draw_rect(fb, 110, 110, 580, 380, 0x333333);
  draw_rect(fb, 120, 120, 560, 360, 0x444444);
  draw_rect(fb, 130, 130, 540, 340, 0x555555);
  draw_rect(fb, 140, 140, 520, 320, 0x666666);
  draw_rect(fb, 150, 150, 500, 300, 0x777777);
  draw_rect(fb, 160, 160, 480, 280, 0x888888);
  draw_rect(fb, 170, 170, 460, 260, 0x999999);
  draw_rect(fb, 180, 180, 440, 240, 0xAAAAAA);
  draw_rect(fb, 190, 190, 420, 220, 0xBBBBBB);
  draw_rect(fb, 200, 200, 400, 200, 0xCCCCCC);
  draw_rect(fb, 210, 210, 380, 180, 0xDDDDDD);
  draw_rect(fb, 220, 220, 360, 160, 0xEEEEEE);
  draw_rect(fb, 230, 230, 340, 140, 0xFFFFFF);
}