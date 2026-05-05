#include "../lib/string.hpp"
#include "allocator.hpp"
#include "arch/gdt.hpp"
#include "doom/doomgeneric/doomgeneric.h"
#include "drivers/mouse.hpp"
#include "idt.hpp"
#include "io.hpp"
#include "limine.h"
#include "pic.hpp"
#include "scheduler.hpp"
#include "terminal.hpp"
#include "ui/compositor.hpp"
#include "ui/components/comp.hpp"
#include "ui/ui.hpp"
#include <stddef.h>
#include <stdint.h>
extern void draw_installer(limine_framebuffer *fb);

extern "C" void keyboard_isr();
extern "C" void timer_isr();
extern "C" void mouse_isr();
extern "C" void exc0();
extern "C" void exc1();
extern "C" void exc2();
extern "C" void exc3();
extern "C" void exc4();
extern "C" void exc5();
extern "C" void exc6();
extern "C" void exc7();
extern "C" void exc8();
extern "C" void exc9();
extern "C" void exc10();
extern "C" void exc11();
extern "C" void exc12();
extern "C" void exc13();
extern "C" void exc14();
extern "C" void exc15();
extern "C" void exc16();
extern "C" void exc17();
extern "C" void exc18();
extern "C" void exc19();
extern "C" void exc20();
extern "C" void exc21();
extern "C" void exc22();
extern "C" void exc23();
extern "C" void exc24();
extern "C" void exc25();
extern "C" void exc26();
extern "C" void exc27();
extern "C" void exc28();
extern "C" void exc29();
extern "C" void exc30();
extern "C" void exc31();

extern Terminal *global_term;

struct cpu_status_t {
  uint64_t rax, rcx, rdx, rsi, rdi, r8, r9, r10, r11, rbx, rbp, r12, r13, r14,
      r15;
  uint64_t vector, error_code, rip, cs, rflags, rsp, ss;
};

extern "C" void itoa(uint64_t n, char *s, int base) {
  static char digits[] = "0123456789ABCDEF";
  char *p = s;
  if (base == 10 && (int64_t)n < 0) {
    *p++ = '-';
    n = (uint64_t)(-(int64_t)n);
  }

  char *start = p;
  do {
    *p++ = digits[n % base];
    n /= base;
  } while (n);
  *p = '\0';
  char *end = p - 1;
  while (start < end) {
    char t = *start;
    *start = *end;
    *end = t;
    start++;
    end--;
  }
}

void serial_init() {
  outb(0x3F8 + 1, 0x00);
  outb(0x3F8 + 3, 0x80);
  outb(0x3F8 + 0, 0x03);
  outb(0x3F8 + 1, 0x00);
  outb(0x3F8 + 3, 0x03);
  outb(0x3F8 + 2, 0xC7);
  outb(0x3F8 + 4, 0x0B);
}

extern "C" void serial_print(const char *s) {
  while (*s) {
    while (!(inb(0x3F8 + 5) & 0x20))
      ;
    outb(0x3F8, *s++);
  }
}

extern "C" volatile struct limine_framebuffer_request fb_request;

static inline uint64_t canonicalize_va48(uint64_t addr) {
  return (uint64_t)((int64_t)(addr << 16) >> 16);
}

extern "C" void exception_handler(cpu_status_t *status) {
  uint64_t cr2;
  __asm__ volatile("mov %%cr2, %0" : "=r"(cr2));
  serial_print("\nEXCEPTION: ");
  char buf[32];
  itoa(status->vector, buf, 10);
  serial_print(buf);
  serial_print(" RIP: ");
  itoa(status->rip, buf, 16);
  serial_print(buf);
  serial_print(" CR2: ");
  itoa(cr2, buf, 16);
  serial_print(buf);
  serial_print("\n");

  while (1) {
    __asm__("cli; hlt");
  }
}

Terminal *global_term = nullptr;
bool in_gui_mode = true;
bool launch_doom = false;
bool doom_running = false;

struct KeyEvent {
  bool pressed;
  uint8_t scancode;
};
static KeyEvent key_buffer[256];
static int key_head = 0, key_tail = 0;

extern "C" __attribute__((target("general-regs-only"))) void
keyboard_handler() {
  uint8_t scancode = inb(0x60);
  bool pressed = !(scancode & 0x80);
  scancode &= 0x7F;
  key_buffer[key_head] = {pressed, scancode};
  key_head = (key_head + 1) % 256;
  outb(0x20, 0x20);
}

extern "C" bool get_next_key(int *pressed, uint8_t *scancode) {
  if (key_head == key_tail)
    return false;
  *pressed = key_buffer[key_tail].pressed;
  *scancode = key_buffer[key_tail].scancode;
  key_tail = (key_tail + 1) % 256;
  return true;
}

volatile uint32_t timer_ticks = 0;
extern "C" uint32_t get_timer_ticks() { return timer_ticks; }

extern "C" __attribute__((target("general-regs-only"))) void timer_handler() {
  outb(0x20, 0x20);
}

extern "C" __attribute__((target("general-regs-only"))) void mouse_handler() {
  mouse_handle_irq();
  outb(0xA0, 0x20);
  outb(0x20, 0x20);
}

static void pit_init(uint32_t freq) {
  uint32_t divisor = 1193182 / freq;
  outb(0x43, 0x36);
  outb(0x40, (uint8_t)(divisor & 0xFF));
  outb(0x40, (uint8_t)((divisor >> 8) & 0xFF));
}

#define LIMINE_ATTR_START __attribute__((used, section(".limine_reqs_start")))
#define LIMINE_ATTR_REQ __attribute__((used, section(".limine_reqs")))
#define LIMINE_ATTR_END __attribute__((used, section(".limine_reqs_end")))

extern "C" {
LIMINE_ATTR_START static volatile uint64_t _limine_start[4] = {
    0xf6b8f4b39de7d1ae, 0xfab91a6940fcb9cf, 0x785c6ed015d3e316,
    0x181e920a7852b9d9};
LIMINE_ATTR_REQ static volatile uint64_t _limine_base_rev[3] = {
    0xf9562b2d5c95a6c8, 0x6a7b384944536bdc, 2};
LIMINE_ATTR_REQ volatile struct limine_framebuffer_request fb_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST, .revision = 0, .response = NULL};
LIMINE_ATTR_REQ volatile struct limine_module_request module_request = {
    .id = LIMINE_MODULE_REQUEST,
    .revision = 0,
    .response = NULL,
    .internal_module_count = 0,
    .internal_modules = NULL};
LIMINE_ATTR_REQ volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST, .revision = 0, .response = NULL};
LIMINE_ATTR_REQ volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST, .revision = 0, .response = NULL};
LIMINE_ATTR_END static volatile uint64_t _limine_end[2] = {0xadc0e0531bb10d03,
                                                           0x9572709f31764c62};
}

uint64_t hhdm_offset = 0;

extern "C" volatile struct limine_module_response *get_module_response() {
  return module_request.response;
}

static uint64_t detect_usable_ram() {
  if (!memmap_request.response)
    return 0;
  uint64_t total = 0;
  for (uint64_t i = 0; i < memmap_request.response->entry_count; i++) {
    struct limine_memmap_entry *e = memmap_request.response->entries[i];
    if (e->type == LIMINE_MEMMAP_USABLE ||
        e->type == LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE ||
        e->type == LIMINE_MEMMAP_ACPI_RECLAIMABLE) {
      total += e->length;
    }
  }
  return total;
}

extern "C" void _start(void) {
  extern char bss_start[];
  extern char bss_end[];
  for (char *p = bss_start; p < bss_end; p++)
    *p = 0;

  serial_init();
  gdt_init();
  if (hhdm_request.response != NULL) {
    hhdm_offset = hhdm_request.response->offset;
  }

  uint64_t ram_bytes = detect_usable_ram();
  in_gui_mode = (ram_bytes >= (128ULL * 1024 * 1024));

  pmm_init();

  if (fb_request.response == NULL ||
      fb_request.response->framebuffer_count < 1) {
    while (1) {
      __asm__("hlt");
    }
  }

  struct limine_framebuffer *fb = fb_request.response->framebuffers[0];
  fb->address = (void *)canonicalize_va48((uint64_t)fb->address);
  screen_width = fb->width;
  screen_height = fb->height;

  struct limine_framebuffer *backbuffer_fb =
      (struct limine_framebuffer *)malloc(sizeof(struct limine_framebuffer));
  memcpy(backbuffer_fb, fb, sizeof(struct limine_framebuffer));

  uint64_t backbuffer_size = fb->pitch * fb->height;
  uint64_t pages_needed = (backbuffer_size + 4095) / 4096;
  void *phys_ptr = pmm_alloc_pages(pages_needed);
  if (!phys_ptr) {
    serial_print("No memory for backbuffer!\n");
    while (1) {
      __asm__("cli; hlt");
    }
  }
  backbuffer_fb->address = (void *)((uint64_t)phys_ptr + hhdm_offset);

  idt_set_descriptor(0, (void *)exc0, 0x8E);
  idt_set_descriptor(1, (void *)exc1, 0x8E);
  idt_set_descriptor(2, (void *)exc2, 0x8E);
  idt_set_descriptor(3, (void *)exc3, 0x8E);
  idt_set_descriptor(4, (void *)exc4, 0x8E);
  idt_set_descriptor(5, (void *)exc5, 0x8E);
  idt_set_descriptor(6, (void *)exc6, 0x8E);
  idt_set_descriptor(7, (void *)exc7, 0x8E);
  idt_set_descriptor(8, (void *)exc8, 0x8E);
  idt_set_descriptor(9, (void *)exc9, 0x8E);
  idt_set_descriptor(10, (void *)exc10, 0x8E);
  idt_set_descriptor(11, (void *)exc11, 0x8E);
  idt_set_descriptor(12, (void *)exc12, 0x8E);
  idt_set_descriptor(13, (void *)exc13, 0x8E);
  idt_set_descriptor(14, (void *)exc14, 0x8E);
  idt_set_descriptor(15, (void *)exc15, 0x8E);
  idt_set_descriptor(16, (void *)exc16, 0x8E);
  idt_set_descriptor(17, (void *)exc17, 0x8E);
  idt_set_descriptor(18, (void *)exc18, 0x8E);
  idt_set_descriptor(19, (void *)exc19, 0x8E);
  idt_set_descriptor(20, (void *)exc20, 0x8E);
  idt_set_descriptor(21, (void *)exc21, 0x8E);
  idt_set_descriptor(22, (void *)exc22, 0x8E);
  idt_set_descriptor(23, (void *)exc23, 0x8E);
  idt_set_descriptor(24, (void *)exc24, 0x8E);
  idt_set_descriptor(25, (void *)exc25, 0x8E);
  idt_set_descriptor(26, (void *)exc26, 0x8E);
  idt_set_descriptor(27, (void *)exc27, 0x8E);
  idt_set_descriptor(28, (void *)exc28, 0x8E);
  idt_set_descriptor(29, (void *)exc29, 0x8E);
  idt_set_descriptor(30, (void *)exc30, 0x8E);
  idt_set_descriptor(31, (void *)exc31, 0x8E);

  idt_set_descriptor(32, (void *)timer_isr, 0x8E);
  idt_set_descriptor(33, (void *)keyboard_isr, 0x8E);
  idt_set_descriptor(44, (void *)mouse_isr, 0x8E);

  idtr_reg.limit = (uint16_t)(sizeof(idt_entry) * 256 - 1);
  idtr_reg.base = (uint64_t)&idt[0];

  pic_init();

  __asm__ volatile("lidt %0" : : "m"(idtr_reg));

  pit_init(100);
  ps2_mouse_init(fb->width, fb->height);

  __asm__ volatile("sti");

  Terminal terminal(fb);
  global_term = &terminal;

  init_ui(fb);

  uint32_t last_tick = 0;
  int last_mx = -1, last_my = -1;
  uint8_t prev_btns = 0;
  bool was_gui = true;

  extern uint32_t *DG_ScreenBuffer;
  static int doom_window_id = -1;
  static bool doom_initialized = false;

  while (1) {
    if (in_gui_mode) {
      was_gui = true;

      bool doom_is_focused =
          (doom_window_id != -1 && Compositor::windows[doom_window_id].focused);

      if (!doom_running || !doom_is_focused) {
        int pressed_k;
        uint8_t sc_k;
        while (get_next_key(&pressed_k, &sc_k)) {
          (void)pressed_k; (void)sc_k;
        }
      }

      uint32_t now = timer_ticks * 10;

      int mx = mouse_x, my = mouse_y;
      uint8_t btns = mouse_buttons;
      uint8_t clicked = ~prev_btns & btns;
      uint8_t released = prev_btns & ~btns;
      prev_btns = btns;

      if (clicked & 0x01) {
        handle_ui_click(backbuffer_fb, mx, my);
      }

      if ((btns & 0x01) && (mx != last_mx || my != last_my)) {
        if (mouse_dragging)
          handle_mouse_drag(mx, my);
        if (mouse_resizing)
          handle_mouse_resize(mx, my);
      }

      last_mx = mx;
      last_my = my;

      if (released & 0x01) {
        end_mouse_drag();
        end_mouse_resize();
      }

      if (now - last_tick >= 16) {
        draw_rect(backbuffer_fb, 0, 0, fb->width, fb->height, 0x0D1117);

        if (installer_available) {
          // ISO boot: only the installer is visible — no desktop
          draw_installer(backbuffer_fb);
        } else {
          // Disk boot: full desktop
          draw_wallpaper_gradient(backbuffer_fb);

          draw_neofetch(backbuffer_fb);
          draw_clock_window(backbuffer_fb);
          draw_calculator_window(backbuffer_fb);
          draw_filemanager_window(backbuffer_fb);

          Compositor::render(backbuffer_fb);

          draw_top_bar(backbuffer_fb);
          draw_taskbar(backbuffer_fb);
          draw_start_menu(backbuffer_fb);
        }

        draw_ui_cursor(backbuffer_fb);
        memcpy(fb->address, backbuffer_fb->address, fb->pitch * fb->height);
        last_tick = now;
      }

      if (launch_doom) {
        launch_doom = false;
        if (!doom_initialized) {
          char *argv[] = {(char *)"doom", (char *)"-iwad", (char *)"DOOM1.WAD",
                          nullptr};
          doomgeneric_Create(3, argv);
          doom_initialized = true;
        }
        doom_running = true;
        if (doom_window_id == -1) {
          doom_window_id = Compositor::create_window(
              100, 100, 640, 400 + MAX_TITLEBAR_HEIGHT, "DOOM", 0x000000, 255);
          Compositor::windows[doom_window_id].content_buffer = DG_ScreenBuffer;
        } else {
          Compositor::focus_window(doom_window_id);
        }
      }

      if (doom_running) {
        bool window_exists = false;
        for (int i = 0; i < Compositor::window_count; i++) {
          if (Compositor::windows[i].content_buffer == DG_ScreenBuffer) {
            window_exists = true;
            doom_window_id = i;
            break;
          }
        }
        if (!window_exists) {
          doom_running = false;
          doom_window_id = -1;
        } else {
          doomgeneric_Tick();
        }
      }
    } else {
      if (was_gui) {
        was_gui = false;
        uint32_t *fb_ptr = (uint32_t *)((uint64_t)fb->address);
        uint64_t total_pixels = (fb->pitch / 4) * fb->height;
        for (uint64_t i = 0; i < total_pixels; i++)
          fb_ptr[i] = 0x000000;
        terminal.cursor_x = 0;
        terminal.cursor_y = 0;
        terminal.print_prompt();
      }

      int pressed;
      uint8_t sc;
      if (get_next_key(&pressed, &sc)) {
        if (pressed && sc < 128 && kbd_map[sc]) {
          terminal.print_char(kbd_map[sc]);
          if (kbd_map[sc] == '\n')
            terminal.print_prompt();
        }
      }
    }
  }
}
