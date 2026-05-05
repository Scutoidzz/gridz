#include <stdint.h>
#include <stddef.h>
#include "../../kernel/fs/fat32.hpp"

/* ── Layout ──────────────────────────────────────────────────────────────── */

#define FM_W            360
#define FM_H            300
#define FM_CONTENT_H    (FM_H - MAX_TITLEBAR_HEIGHT)  // 268
#define FM_PATH_H       20
#define FM_HDR_H        14
// List starts after: path bar + 1px sep + header + 1px sep
#define FM_LIST_Y       (FM_PATH_H + 1 + FM_HDR_H + 1)  // 36
#define FM_SCROLL_W     16
#define FM_LIST_W       (FM_W - FM_SCROLL_W)             // 344
#define FM_LIST_H       (FM_CONTENT_H - FM_LIST_Y)       // 232
#define FM_ENTRY_H      18
#define FM_PER_PAGE     (FM_LIST_H / FM_ENTRY_H)         // 12
#define FM_MAX_ENTRIES  128

/* ── State ───────────────────────────────────────────────────────────────── */

static fat32::DirInfo fm_entries[FM_MAX_ENTRIES];
static int   fm_count      = 0;
static int   fm_scroll     = 0;
static bool  fm_dirty      = true;
static bool  fm_fs_ok      = false;
static bool  fm_fs_tried   = false;
static uint32_t fm_cluster = 2;

// Navigation stack so we can go back up
static uint32_t fm_nav_stack[16];
static int      fm_nav_depth = 0;

static uint32_t fm_buf[FM_W * FM_CONTENT_H];
extern int fm_window_id;

/* ── Helpers ─────────────────────────────────────────────────────────────── */

static int fm_strlen(const char *s) { int n = 0; while (s[n]) n++; return n; }

static void fm_strcpy(char *dst, const char *src) {
    while ((*dst++ = *src++));
}

static bool fm_streq(const char *a, const char *b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return *a == '\0' && *b == '\0';
}

static void fmt_size(uint32_t sz, char *out) {
    if (sz == 0) { out[0]='-'; out[1]='-'; out[2]='\0'; return; }
    uint32_t v;
    const char *unit;
    if (sz < 1024)            { v = sz;             unit = "B";  }
    else if (sz < 1024*1024)  { v = sz / 1024;      unit = "KB"; }
    else                      { v = sz / 1024/1024; unit = "MB"; }
    // u32_to_str equivalent inline
    if (v == 0) { out[0]='0'; out[1]='\0'; }
    else {
        char tmp[12]; int tlen = 0;
        uint32_t vv = v;
        while (vv) { tmp[tlen++] = '0' + (int)(vv % 10); vv /= 10; }
        int i; for (i = 0; i < tlen; i++) out[i] = tmp[tlen-1-i];
        out[i] = '\0';
    }
    int len = fm_strlen(out);
    out[len++] = ' ';
    for (int i = 0; unit[i]; i++) out[len++] = unit[i];
    out[len] = '\0';
}

static void ffil(limine_framebuffer *fb, int x, int y, int w, int h, uint32_t c) {
    draw_rectangle(fb, x, y, w, h, c);
}
static void ftxt(limine_framebuffer *fb, int x, int y, const char *s, uint32_t c) {
    draw_string(fb, x, y, s, c);
}

/* ── Load current directory ──────────────────────────────────────────────── */

static void fm_load() {
    if (!fm_fs_tried) {
        fm_fs_tried = true;
        fm_fs_ok = fat32::init(false) || fat32::init(true);
        if (fm_fs_ok) fm_cluster = fat32::root_cluster();
    }
    if (!fm_fs_ok) { fm_count = 0; return; }
    fm_count = fat32::list_directory(fm_cluster, fm_entries, FM_MAX_ENTRIES);
    fm_dirty = false;
}

/* ── Draw ────────────────────────────────────────────────────────────────── */

void draw_filemanager_window(limine_framebuffer *fb) {
    if (!filemanager_open) {
        if (fm_window_id >= 0) {
            Compositor::destroy_window(fm_window_id);
            fm_window_id = -1;
            // Reset state for next open
            fm_dirty = true;
            fm_scroll = 0;
        }
        return;
    }

    int W = (int)fb->width, H = (int)fb->height;
    int wx = W / 2 - FM_W / 2 - 40;
    int wy = H / 2 - FM_H / 2;

    if (fm_window_id < 0) {
        fm_window_id = Compositor::create_window(wx, wy, FM_W, FM_H,
                                                 "File Manager", 0x111827, 248, 11);
        Compositor::windows[fm_window_id].content_buffer   = fm_buf;
        Compositor::windows[fm_window_id].content_buffer_w = FM_W;
        Compositor::windows[fm_window_id].content_buffer_h = FM_CONTENT_H;
        fm_dirty  = true;
        fm_scroll = 0;
    }

    if (fm_dirty) fm_load();

    limine_framebuffer fake;
    fake.address = fm_buf;
    fake.width   = FM_W;
    fake.height  = FM_CONTENT_H;
    fake.pitch   = FM_W * 4;

    ffil(&fake, 0, 0, FM_W, FM_CONTENT_H, 0x111827);

    // ── Path bar ──────────────────────────────────────────────────────────
    ffil(&fake, 0, 0, FM_W, FM_PATH_H, 0x0D1117);
    if (fm_nav_depth > 0) {
        ftxt(&fake, 6, 6, "[..]", 0x3A8EFF);
    } else {
        ftxt(&fake, 6, 6, "Root", 0x3A8EFF);
    }
    // Depth indicator
    if (fm_nav_depth > 0) {
        char depth_str[4];
        depth_str[0] = ' '; depth_str[1] = '/';
        // show depth as dots
        depth_str[2] = '\0';
        int d = fm_nav_depth;
        if (d > 9) d = 9;
        depth_str[0] = '0' + d;
        depth_str[1] = ' '; depth_str[2] = 'l'; depth_str[3] = 'v';
        char tmp[8]; tmp[0]='L'; tmp[1]='v'; tmp[2]='l'; tmp[3]=' ';
        tmp[4] = '0' + (char)fm_nav_depth; tmp[5]='\0';
        ftxt(&fake, 38, 6, tmp, 0x5588AA);
    }
    // Entry count
    {
        char cnt[12] = "            ";
        // right-align count string
        char num[8]; int pos = 0;
        uint32_t n = (uint32_t)fm_count;
        if (n == 0) { num[pos++]='0'; }
        else { char t[8]; int tl=0; while(n){t[tl++]='0'+(int)(n%10);n/=10;}
               for(int i=tl-1;i>=0;i--) num[pos++]=t[i]; }
        num[pos++]=' '; num[pos++]='i'; num[pos++]='t'; num[pos++]='e';
        num[pos++]='m'; if(fm_count!=1) num[pos++]='s'; num[pos]='\0';
        int tx = FM_W - fm_strlen(num)*8 - 6;
        if (!fm_fs_ok) {
            ftxt(&fake, tx - 24, 6, "No disk", 0xFF6666);
        } else {
            ftxt(&fake, tx, 6, num, 0x4477AA);
        }
    }
    ffil(&fake, 0, FM_PATH_H, FM_W, 1, 0x3A8EFF);

    // ── Column headers ────────────────────────────────────────────────────
    ffil(&fake, 0, FM_PATH_H+1, FM_W, FM_HDR_H, 0x0A1020);
    ftxt(&fake, 16,  FM_PATH_H+3, "Name",  0x4A7AAA);
    ftxt(&fake, 248, FM_PATH_H+3, "Size",  0x4A7AAA);
    ftxt(&fake, 314, FM_PATH_H+3, "Type",  0x4A7AAA);
    ffil(&fake, 0, FM_PATH_H+1+FM_HDR_H, FM_W, 1, 0x1A2A3A);

    // ── Scroll buttons ────────────────────────────────────────────────────
    int sx = FM_LIST_W;
    ffil(&fake, sx, FM_LIST_Y, FM_SCROLL_W, FM_LIST_H, 0x0D1520);
    // Up button
    ffil(&fake, sx, FM_LIST_Y, FM_SCROLL_W, 16, fm_scroll > 0 ? 0x1A3A5A : 0x0D1520);
    ftxt(&fake, sx+4, FM_LIST_Y+4, "^", fm_scroll > 0 ? 0x7FCBFF : 0x334455);
    // Down button
    int can_dn = (fm_scroll + FM_PER_PAGE < fm_count);
    ffil(&fake, sx, FM_LIST_Y + FM_LIST_H - 16, FM_SCROLL_W, 16,
         can_dn ? 0x1A3A5A : 0x0D1520);
    ftxt(&fake, sx+4, FM_LIST_Y + FM_LIST_H - 12, "v",
         can_dn ? 0x7FCBFF : 0x334455);
    // Scroll track + thumb
    if (fm_count > FM_PER_PAGE) {
        int track_h  = FM_LIST_H - 32;
        int thumb_h  = track_h * FM_PER_PAGE / fm_count;
        if (thumb_h < 6) thumb_h = 6;
        int thumb_y  = FM_LIST_Y + 16 +
                       (track_h - thumb_h) * fm_scroll / (fm_count - FM_PER_PAGE);
        ffil(&fake, sx+2, FM_LIST_Y+16, FM_SCROLL_W-4, track_h, 0x0D1117);
        ffil(&fake, sx+2, thumb_y, FM_SCROLL_W-4, thumb_h, 0x2A4A6A);
    }

    // ── File list ─────────────────────────────────────────────────────────
    int visible = fm_count - fm_scroll;
    if (visible > FM_PER_PAGE) visible = FM_PER_PAGE;

    for (int i = 0; i < visible; i++) {
        int idx = fm_scroll + i;
        fat32::DirInfo &e = fm_entries[idx];
        int ey  = FM_LIST_Y + i * FM_ENTRY_H;
        bool dot = (e.name[0] == '.' && (e.name[1] == '\0' ||
                   (e.name[1] == '.' && e.name[2] == '\0')));

        // Row background (alternating)
        uint32_t row_bg = (i % 2 == 0) ? 0x111827 : 0x0F1520;
        ffil(&fake, 0, ey, FM_LIST_W, FM_ENTRY_H, row_bg);

        // Icon
        uint32_t icon_col = e.is_dir ? 0xFFCC44 : 0x5599CC;
        if (dot) icon_col = 0x4A5A6A;
        ffil(&fake, 4, ey+4, 8, 10, icon_col);
        if (e.is_dir) ffil(&fake, 4, ey+4, 8, 3, icon_col + 0x222222); // folder tab

        // Name
        uint32_t name_col = dot ? 0x4477AA : (e.is_dir ? 0xFFCC44 : 0xDDEEFF);
        ftxt(&fake, 16, ey+5, e.name, name_col);

        // Size (files only)
        if (!e.is_dir && !dot) {
            char sz[12];
            fmt_size(e.size, sz);
            int sx2 = 248 + (60 - fm_strlen(sz)*8);
            if (sx2 < 248) sx2 = 248;
            ftxt(&fake, sx2, ey+5, sz, 0x7799AA);
        }

        // Type badge
        if (e.is_dir) {
            ftxt(&fake, 316, ey+5, "DIR", 0xFFCC44);
        } else if (!dot) {
            // Show extension
            const char *ext = e.name;
            const char *dot_pos = nullptr;
            for (const char *p = e.name; *p; p++) if (*p == '.') dot_pos = p;
            if (dot_pos) ext = dot_pos + 1;
            else ext = "   ";
            ftxt(&fake, 316, ey+5, ext, 0x5588AA);
        }

        // Row separator
        ffil(&fake, 0, ey + FM_ENTRY_H - 1, FM_LIST_W, 1, 0x1A2A3A);
    }

    // Empty state
    if (fm_count == 0) {
        const char *msg = fm_fs_ok ? "Empty directory" : "No FAT32 disk found";
        int tx = FM_W/2 - fm_strlen(msg)*4;
        ftxt(&fake, tx, FM_LIST_Y + FM_LIST_H/2 - 4, msg, 0x445566);
    }
}

/* ── Click handler ───────────────────────────────────────────────────────── */

void handle_filemanager_click(int win_id, int mx, int my) {
    if (win_id < 0 || win_id >= Compositor::window_count) return;
    Window *win = &Compositor::windows[win_id];

    int cx = mx - win->x;
    int cy = my - (win->y + MAX_TITLEBAR_HEIGHT);

    // Scroll buttons
    if (cx >= FM_LIST_W) {
        if (cy >= FM_LIST_Y && cy < FM_LIST_Y + 16) {
            if (fm_scroll > 0) { fm_scroll--; }
            return;
        }
        if (cy >= FM_LIST_Y + FM_LIST_H - 16 && cy < FM_LIST_Y + FM_LIST_H) {
            if (fm_scroll + FM_PER_PAGE < fm_count) { fm_scroll++; }
            return;
        }
        return;
    }

    // Path bar: click "[..]" to go up
    if (cy < FM_PATH_H) {
        if (fm_nav_depth > 0) {
            fm_cluster = fm_nav_stack[--fm_nav_depth];
            fm_scroll = 0;
            fm_dirty  = true;
        }
        return;
    }

    // File list rows
    if (cy < FM_LIST_Y) return;
    int row = (cy - FM_LIST_Y) / FM_ENTRY_H;
    if (row < 0 || row >= FM_PER_PAGE) return;
    int idx = fm_scroll + row;
    if (idx < 0 || idx >= fm_count) return;

    fat32::DirInfo &e = fm_entries[idx];

    if (e.name[0] == '.' && e.name[1] == '.' && e.name[2] == '\0') {
        // Go up
        if (fm_nav_depth > 0) {
            fm_cluster = fm_nav_stack[--fm_nav_depth];
            fm_scroll  = 0;
            fm_dirty   = true;
        }
        return;
    }

    if (e.name[0] == '.' && e.name[1] == '\0') return; // "." — ignore

    if (e.is_dir && e.cluster != 0) {
        if (fm_nav_depth < 16) {
            fm_nav_stack[fm_nav_depth++] = fm_cluster;
        }
        fm_cluster = e.cluster;
        fm_scroll  = 0;
        fm_dirty   = true;
    }
}
