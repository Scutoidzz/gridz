#include "app_manager.hpp"
#include "terminal.hpp"

extern Terminal *global_term;

App AppManager::registered_apps[MAX_APPS];
int AppManager::app_count = 0;

void AppManager::init() {
  register_app("Clock", "A simple clock app", "apps/clock/clock.py", 0xFFFF00);
  register_app("Files", "File Explorer", "apps/files/main.py", 0x00FF00);
  register_app("Doom", "Run Doom", "apps/doom/start.py", 0xFF00FF);
}

void AppManager::register_app(const char *name, const char *description,
                              const char *entry_point, uint32_t icon_color) {
  if (app_count < MAX_APPS) {
    registered_apps[app_count].name = name;
    registered_apps[app_count].description = description;
    registered_apps[app_count].entry_point = entry_point;
    registered_apps[app_count].icon_color = icon_color;
    app_count++;
  }
}

void AppManager::draw_apps(limine_framebuffer *fb) {
  int start_x = 150; // Distance from the Start button
  int icon_y = fb->height - 35;

  for (int i = 0; i < app_count; i++) {
    // Draw the app icon (rounded square)
    draw_rectangle(fb, start_x + (i * 45), icon_y, 30, 30,
                   registered_apps[i].icon_color);

    // Draw the app's first letter as an icon label
    char initial[2] = {registered_apps[i].name[0], '\0'};
    draw_string(fb, start_x + (i * 45) + 11, icon_y + 11, initial, 0x000000);
  }
}

extern "C" void execute_python_file(const char *path);

void AppManager::run_app(int index) {
  if (index >= 0 && index < app_count) {
    if (global_term) {
      global_term->print("Launching ");
      global_term->print(registered_apps[index].name);
      global_term->print("...\n");
    }
    const char *entry = registered_apps[index].entry_point;
    int len = 0;
    while (entry[len])
      len++;
    if (len > 4 && entry[len - 4] == "." && entry[len - 3] == "s") {
      if (global_term)
        global_term->print("SDE: Executing No-Code package...\n");
    }
  }
}

void AppManager::handle_click(int mx, int my) {
  int start_x = 150;

  for (int i = 0; i < app_count; i++) {
    int icon_x = start_x + (i * 45);
    if (mx >= icon_x && mx <= icon_x + 30) {
      run_app(i);
      break;
    }
  }
}
