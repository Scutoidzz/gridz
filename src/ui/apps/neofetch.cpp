
#include <stddef.h>
#include <stdint.h>



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
  txt(&fake_fb, tx, ty, "OS:   Gridz 0.1", 0x7FCBFF);
  ty += 14;

  char cpu[49] = "";
  get_cpu_brand(cpu);
  char cpu_line[64] = "CPU:  ";
  int pos = 6;
  str_append(cpu_line, &pos, cpu, 62);
  txt(&fake_fb, tx, ty, cpu_line, 0xAADDFF);
  ty += 14;

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