#include "ui/ui.hpp"
#include "kernel/drivers/ata.hpp"
#include "kernel/fs/fat32.hpp"
#include "limine.h"

static bool installing = false;
static int install_progress = 0;
static bool selected_slave = false;
static bool drive_master_exists = false;
static bool drive_slave_exists = false;

static char master_model[41] = {0};
static uint32_t master_mb = 0;
static char slave_model[41] = {0};
static uint32_t slave_mb = 0;

/* External UI helpers */
extern int draw_window(limine_framebuffer* fb, int x, int y, int w, int h, const char* title, uint32_t title_color);
extern bool hit_close(int px, int py, int wx, int wy, int ww);
extern int g_w, g_h;
extern bool installer_open;
extern "C" void serial_print(const char* s);
extern "C" volatile struct limine_module_response* get_module_response();

static bool strcmp(const char* s1, const char* s2) {
    while(*s1 && (*s1 == *s2)) { s1++; s2++; }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2 == 0;
}

static void u32_to_str(uint32_t v, char* buf) {
    if (!v) { buf[0] = '0'; buf[1] = '\0'; return; }
    char tmp[12];
    int len = 0;
    while (v) { tmp[len++] = '0' + v % 10; v /= 10; }
    for (int i = 0; i < len; i++) buf[i] = tmp[len - 1 - i];
    buf[len] = '\0';
}

static void get_drive_info(bool slave, char* out_model, uint32_t* out_mb) {
    uint16_t buffer[256];
    if (!ata::identify(buffer, slave)) {
        out_model[0] = '\0';
        *out_mb = 0;
        return;
    }
    for (int i = 0; i < 20; i++) {
        uint16_t word = buffer[27 + i];
        out_model[i * 2] = (char)(word >> 8);
        out_model[i * 2 + 1] = (char)(word & 0xFF);
    }
    out_model[40] = '\0';
    for (int i = 39; i >= 0 && (out_model[i] == ' ' || out_model[i] == '\0'); i--) out_model[i] = '\0';
    uint32_t sectors = buffer[60] | (buffer[61] << 16);
    *out_mb = sectors / 2048;
}

void draw_installer(limine_framebuffer* fb) {
    if (!installer_open) return;

    if (!installing) {
        serial_print("[DBG] Installer: Scanning for drives...\n");
        drive_master_exists = ata::exists(false);
        if (drive_master_exists) {
            serial_print("[DBG] Installer: Primary Master Found\n");
            get_drive_info(false, master_model, &master_mb);
        } else {
            serial_print("[DBG] Installer: Primary Master NOT found\n");
        }
        
        drive_slave_exists = ata::exists(true);
        if (drive_slave_exists) {
            serial_print("[DBG] Installer: Primary Slave Found\n");
            get_drive_info(true, slave_model, &slave_mb);
        } else {
            serial_print("[DBG] Installer: Primary Slave NOT found\n");
        }
    }

    int ww = 550, wh = 380;
    int wx = (g_w - ww) / 2;
    int wy = (g_h - wh) / 2;
    int iy = draw_window(fb, wx, wy, ww, wh, "Gridz OS Installer", 0x2A6EBB);

    if (!installing) {
        draw_string(fb, wx + 20, iy + 20, "Select a target drive for installation:", 0xFFFFFF);

        // Master Drive Panel
        uint32_t m_color = drive_master_exists ? (selected_slave ? 0x1A1A2E : 0x3A8EFF) : 0x333333;
        draw_rect(fb, wx + 20, iy + 50, ww - 40, 60, m_color);
        if (drive_master_exists) {
            draw_string(fb, wx + 35, iy + 62, master_model, 0xFFFFFF);
            char size_str[32] = "Capacity: ";
            char val[12]; u32_to_str(master_mb, val);
            int p = 10; while(val[p-10]) { size_str[p] = val[p-10]; p++; }
            size_str[p++] = ' '; size_str[p++] = 'M'; size_str[p++] = 'B'; size_str[p] = '\0';
            draw_string(fb, wx + 35, iy + 82, size_str, 0xCCCCCC);
        } else {
            draw_string(fb, wx + 35, iy + 72, "Primary Master: Empty", 0x888888);
        }

        // Slave Drive Panel
        uint32_t s_color = drive_slave_exists ? (selected_slave ? 0x3A8EFF : 0x1A1A2E) : 0x333333;
        draw_rect(fb, wx + 20, iy + 120, ww - 40, 60, s_color);
        if (drive_slave_exists) {
            draw_string(fb, wx + 35, iy + 132, slave_model, 0xFFFFFF);
            char size_str[32] = "Capacity: ";
            char val[12]; u32_to_str(slave_mb, val);
            int p = 10; while(val[p-10]) { size_str[p] = val[p-10]; p++; }
            size_str[p++] = ' '; size_str[p++] = 'M'; size_str[p++] = 'B'; size_str[p] = '\0';
            draw_string(fb, wx + 35, iy + 152, size_str, 0xCCCCCC);
        } else {
            draw_string(fb, wx + 35, iy + 142, "Primary Slave: Empty", 0x888888);
        }

        if (drive_master_exists || drive_slave_exists) {
            draw_rect(fb, wx + 175, iy + 250, 200, 50, 0x1A3A5C);
            draw_string(fb, wx + 230, iy + 268, "INSTALL NOW", 0xFFFFFF);
        }
    } else {
        draw_string(fb, wx + 20, iy + 20, "Installing Gridz OS to Disk...", 0xFFFFFF);
        
        draw_rect(fb, wx + 20, iy + 80, ww - 40, 25, 0x111111);
        int progress_w = ((ww - 40) * install_progress) / 100;
        draw_rect(fb, wx + 20, iy + 80, progress_w, 25, 0x3A8EFF);

        if (install_progress < 100) {
            auto res = get_module_response();
            if (res) {
                for (uint64_t i = 0; i < res->module_count; i++) {
                    if (strcmp(res->modules[i]->cmdline, "install_bootblock") == 0) {
                        // Write bootblock (10MB = 20480 sectors)
                        // We write it in chunks to avoid blocking too long
                        uint16_t* data = (uint16_t*)res->modules[i]->address;
                        uint32_t total_sectors = res->modules[i]->size / 512;
                        uint32_t step = total_sectors / 50;
                        uint32_t start = (install_progress * total_sectors) / 100;
                        uint32_t end = ((install_progress + 2) * total_sectors) / 100;
                        
                        for (uint32_t s = start; s < end && s < total_sectors; s++) {
                            ata::write_sectors(s, 1, data + (s * 256), selected_slave);
                        }
                    }
                }
            }
            install_progress += 2;
        } else {
            draw_string(fb, wx + 20, iy + 130, "Installation Complete!", 0x00FF00);
            draw_string(fb, wx + 20, iy + 150, "The system was successfully deployed to your drive.", 0xFFFFFF);
            draw_string(fb, wx + 20, iy + 170, "You can now reboot and run the OS directly.", 0xAAAAAA);
            
            draw_rect(fb, wx + 175, iy + 250, 200, 50, 0x1A5C1A);
            draw_string(fb, wx + 220, iy + 268, "REBOOT NOW", 0xFFFFFF);
        }
    }
}

bool handle_installer_click(limine_framebuffer* fb, int mx, int my) {
    if (!installer_open) return false;
    int ww = 550, wh = 380;
    int wx = (g_w - ww) / 2;
    int wy = (g_h - wh) / 2;
    int iy = wy + 22;

    if (hit_close(mx, my, wx, wy, ww)) {
        installer_open = false;
        draw_taskbar(fb); draw_top_bar(fb);
        return true;
    }

    if (!installing) {
        if (mx >= wx + 20 && mx <= wx + ww - 20) {
            if (my >= iy + 50 && my <= iy + 110 && drive_master_exists) { selected_slave = false; return true; }
            if (my >= iy + 120 && my <= iy + 180 && drive_slave_exists) { selected_slave = true; return true; }
        }
        if (mx >= wx + 175 && mx <= wx + 375 && my >= iy + 250 && my <= iy + 300) {
            if ((selected_slave && drive_slave_exists) || (!selected_slave && drive_master_exists)) {
                installing = true; install_progress = 0;
            }
            return true;
        }
    } else if (install_progress >= 100) {
        if (mx >= wx + 175 && mx <= wx + 375 && my >= iy + 250 && my <= iy + 300) __asm__ volatile("cli; hlt");
    }
    return true;
}
