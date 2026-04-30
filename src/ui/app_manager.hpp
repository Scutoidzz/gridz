#pragma once
#include "components/comp.hpp"
#include <stdint.h>

struct App {
  const char *name;
  const char *description;
  const char *entry_point;
  uint32_t icon_color;
};

class AppManager {
public:
  static const int MAX_APPS = 5;
  static App registered_apps[MAX_APPS];
  static int app_count;

  static void init();
  static void register_app(const char *name, const char *description,
                           const char *entry_point, uint32_t icon_color);
  static void draw_apps(limine_framebuffer *fb);
  static void handle_click(int x, int y);
  static void run_app(int index);
};
