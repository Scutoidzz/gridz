#include <stdint.h>
#include <stddef.h>

/* ── Fixed-point arithmetic (no FPU) ────────────────────────────────────────
 * All values are stored as actual_value * SCALE (6 decimal places).
 * Example: 3.14 is stored as 3,140,000.
 */

typedef int64_t Num;
// 3 decimal places; safe up to actual values ~±3,000,000
static const int64_t CALC_SCALE = 1000LL;

static Num num_add(Num a, Num b) { return a + b; }
static Num num_sub(Num a, Num b) { return a - b; }

static Num num_mul(Num a, Num b) {
    // a*b/SCALE - safe when stored values ≤ ~3e9 (actual ≤ ~3e6)
    return a * b / CALC_SCALE;
}

static Num num_div(Num a, Num b) {
    if (b == 0) return 0;
    // result = (a/b)*SCALE + (a%b)*SCALE/b
    int64_t q = a / b;
    int64_t r = a % b;
    int64_t frac = (r <= (INT64_MAX / CALC_SCALE)) ? r * CALC_SCALE / b : 0;
    return q * CALC_SCALE + frac;
}

static Num num_parse(const char *s) {
    bool neg = false;
    if (*s == '-') { neg = true; s++; }

    int64_t int_part = 0;
    while (*s >= '0' && *s <= '9') { int_part = int_part * 10 + (*s - '0'); s++; }

    int64_t frac = 0;
    int frac_digits = 0;
    if (*s == '.') {
        s++;
        while (*s >= '0' && *s <= '9' && frac_digits < 6) {
            frac = frac * 10 + (*s - '0');
            frac_digits++;
            s++;
        }
    }
    // Limit input to 3 decimal places (matches SCALE)
    while (frac_digits > 3) { frac /= 10; frac_digits--; }
    int64_t scale = CALC_SCALE;
    for (int i = 0; i < frac_digits; i++) scale /= 10;

    Num result = int_part * CALC_SCALE + frac * scale;
    return neg ? -result : result;
}

static void num_to_str(Num n, char *buf) {
    int pos = 0;
    if (n < 0) { buf[pos++] = '-'; n = -n; }

    int64_t int_part  = (int64_t)(n / CALC_SCALE);
    int64_t frac_part = (int64_t)(n % CALC_SCALE);

    if (int_part == 0) {
        buf[pos++] = '0';
    } else {
        char tmp[20]; int tlen = 0;
        int64_t ip = int_part;
        while (ip) { tmp[tlen++] = '0' + (int)(ip % 10); ip /= 10; }
        for (int i = tlen - 1; i >= 0; i--) buf[pos++] = tmp[i];
    }

    if (frac_part > 0) {
        buf[pos++] = '.';
        char fstr[4]; // 3 decimal digits
        int64_t fp = frac_part;
        for (int i = 2; i >= 0; i--) { fstr[i] = '0' + (int)(fp % 10); fp /= 10; }
        int flen = 3;
        while (flen > 0 && fstr[flen - 1] == '0') flen--;
        for (int i = 0; i < flen; i++) buf[pos++] = fstr[i];
    }

    buf[pos] = '\0';
}

/* ── Calculator state ────────────────────────────────────────────────────── */

static Num  calc_acc  = 0;
static char calc_display[16] = "0";
static int  calc_display_len = 1;
static char calc_op   = 0;     // pending operator: + - * / or 0
static bool calc_fresh = true; // next digit starts a new number
static bool calc_has_dot = false;

static int calc_str_len(const char *s) {
    int n = 0; while (s[n]) n++; return n;
}

static void calc_set_display(Num n) {
    num_to_str(n, calc_display);
    calc_display_len = calc_str_len(calc_display);
    calc_has_dot = false;
    for (int i = 0; calc_display[i]; i++)
        if (calc_display[i] == '.') { calc_has_dot = true; break; }
}

static void calc_reset() {
    calc_acc = 0;
    calc_display[0] = '0'; calc_display[1] = '\0';
    calc_display_len = 1;
    calc_op = 0;
    calc_fresh = true;
    calc_has_dot = false;
}

static Num calc_apply(Num a, Num b, char op) {
    switch (op) {
        case '+': return num_add(a, b);
        case '-': return num_sub(a, b);
        case '*': return num_mul(a, b);
        case '/': return num_div(a, b);
    }
    return b;
}

/* ── Layout constants ────────────────────────────────────────────────────── */

#define CALC_W      220
#define CALC_H      250
#define CALC_DISP_Y 4
#define CALC_DISP_H 36
#define CALC_BTN_Y  48
#define CALC_ROWS   5
#define CALC_COLS   4
#define CALC_BTN_H  32
#define CALC_BTN_W  53
#define CALC_GAP    2

static const char* CALC_LABELS[CALC_ROWS][CALC_COLS] = {
    {"C",  "BS", "+/-", "/"},
    {"7",  "8",  "9",   "*"},
    {"4",  "5",  "6",   "-"},
    {"1",  "2",  "3",   "+"},
    {"0",  "0",  ".",   "="},
};

static uint32_t calc_buf[CALC_W * (CALC_H - MAX_TITLEBAR_HEIGHT)];
extern int calc_window_id;

static void cfil(limine_framebuffer *fb, int x, int y, int w, int h, uint32_t c) {
    draw_rectangle(fb, x, y, w, h, c);
}
static void ctxt(limine_framebuffer *fb, int x, int y, const char *s, uint32_t c) {
    draw_string(fb, x, y, s, c);
}

static uint32_t btn_bg_color(int row, int col) {
    const char *lbl = CALC_LABELS[row][col];
    if (lbl[0] == 'C')                                     return 0x5A1A1A;
    if (lbl[0] == 'B')                                     return 0x3A2A1A;
    if (lbl[0] == '=')                                     return 0x1A5A2A;
    if (lbl[0] == '/' || lbl[0] == '*' ||
        (lbl[0] == '-' && lbl[1] == '\0') ||
        (lbl[0] == '+' && lbl[1] == '\0'))                 return 0x1A3A6A;
    return 0x2A3A4A;
}

static uint32_t btn_fg_color(int row, int col) {
    const char *lbl = CALC_LABELS[row][col];
    if (lbl[0] == 'C')                                     return 0xFFAAAA;
    if (lbl[0] == 'B')                                     return 0xFFCCAA;
    if (lbl[0] == '=')                                     return 0xAAFFAA;
    if (lbl[0] == '/' || lbl[0] == '*' ||
        (lbl[0] == '-' && lbl[1] == '\0') ||
        (lbl[0] == '+' && lbl[1] == '\0'))                 return 0x7FCBFF;
    return 0xDDEEFF;
}

/* ── Draw ────────────────────────────────────────────────────────────────── */

void draw_calculator_window(limine_framebuffer *fb) {
    if (!calculator_open) {
        if (calc_window_id >= 0) {
            Compositor::destroy_window(calc_window_id);
            calc_window_id = -1;
        }
        return;
    }

    int W = (int)fb->width, H = (int)fb->height;
    int wx = W / 2 - CALC_W / 2 + 60;
    int wy = H / 2 - CALC_H / 2 - 30;

    if (calc_window_id < 0) {
        calc_window_id = Compositor::create_window(wx, wy, CALC_W, CALC_H,
                                                   "Calculator", 0x1A2A3A, 245, 12);
        Compositor::windows[calc_window_id].content_buffer   = calc_buf;
        Compositor::windows[calc_window_id].content_buffer_w = CALC_W;
        Compositor::windows[calc_window_id].content_buffer_h = CALC_H - MAX_TITLEBAR_HEIGHT;
    }

    limine_framebuffer fake;
    fake.address = calc_buf;
    fake.width   = CALC_W;
    fake.height  = CALC_H - MAX_TITLEBAR_HEIGHT;
    fake.pitch   = CALC_W * 4;

    int cH = CALC_H - MAX_TITLEBAR_HEIGHT;
    cfil(&fake, 0, 0, CALC_W, cH, 0x1A2A3A);

    // Display area
    cfil(&fake, 2, CALC_DISP_Y,     CALC_W - 4, CALC_DISP_H, 0x0D1520);
    cfil(&fake, 2, CALC_DISP_Y,     CALC_W - 4, 1,           0x3A6080);
    cfil(&fake, 2, CALC_DISP_Y + CALC_DISP_H - 1, CALC_W - 4, 1, 0x3A6080);

    // Right-align display text
    int dlen = calc_str_len(calc_display);
    int tx = CALC_W - 6 - dlen * 8;
    if (tx < 4) tx = 4;
    ctxt(&fake, tx, CALC_DISP_Y + 12, calc_display, 0x7FCBFF);

    // Operator indicator
    if (calc_op) {
        char op_str[2] = {calc_op, '\0'};
        ctxt(&fake, 4, CALC_DISP_Y + 12, op_str, 0x4488AA);
    }

    // Buttons
    for (int row = 0; row < CALC_ROWS; row++) {
        for (int col = 0; col < CALC_COLS; col++) {
            int bx = col * (CALC_BTN_W + CALC_GAP) + 1;
            int by = CALC_BTN_Y + row * (CALC_BTN_H + CALC_GAP);
            uint32_t bg = btn_bg_color(row, col);
            cfil(&fake, bx, by, CALC_BTN_W, CALC_BTN_H, bg);
            cfil(&fake, bx, by, CALC_BTN_W, 1, 0x405060); // top highlight

            const char *lbl = CALC_LABELS[row][col];
            int llen = calc_str_len(lbl);
            int ltx = bx + (CALC_BTN_W - llen * 8) / 2;
            int lty = by + (CALC_BTN_H - 8) / 2;
            ctxt(&fake, ltx, lty, lbl, btn_fg_color(row, col));
        }
    }
}

/* ── Click handler ───────────────────────────────────────────────────────── */

void handle_calculator_click(int win_id, int mx, int my) {
    if (win_id < 0 || win_id >= Compositor::window_count) return;
    Window *win = &Compositor::windows[win_id];

    int cx = mx - win->x;
    int cy = my - (win->y + MAX_TITLEBAR_HEIGHT);

    if (cy < CALC_BTN_Y) return;
    int row = (cy - CALC_BTN_Y) / (CALC_BTN_H + CALC_GAP);
    int col = cx / (CALC_BTN_W + CALC_GAP);
    if (row < 0 || row >= CALC_ROWS || col < 0 || col >= CALC_COLS) return;

    const char *lbl = CALC_LABELS[row][col];

    if (lbl[0] == 'C') {
        calc_reset();
        return;
    }

    if (lbl[0] == 'B') { // backspace
        if (!calc_fresh && calc_display_len > 0) {
            if (calc_display[calc_display_len - 1] == '.') calc_has_dot = false;
            calc_display[--calc_display_len] = '\0';
            if (calc_display_len == 0 ||
                (calc_display_len == 1 && calc_display[0] == '-')) {
                calc_display[0] = '0'; calc_display[1] = '\0';
                calc_display_len = 1;
                calc_fresh = true;
            }
        }
        return;
    }

    if (lbl[0] == '+' && lbl[1] == '/') { // +/-
        if (calc_display[0] == '-') {
            for (int i = 0; i < calc_display_len; i++)
                calc_display[i] = calc_display[i + 1];
            calc_display_len--;
        } else if (calc_display[0] != '0' || calc_display_len > 1) {
            for (int i = calc_display_len; i >= 0; i--)
                calc_display[i + 1] = calc_display[i];
            calc_display[0] = '-';
            calc_display_len++;
        }
        return;
    }

    if (lbl[0] >= '0' && lbl[0] <= '9') {
        if (calc_fresh) {
            calc_display[0] = lbl[0]; calc_display[1] = '\0';
            calc_display_len = 1;
            calc_fresh = false;
            calc_has_dot = false;
        } else if (calc_display_len < 14) {
            calc_display[calc_display_len++] = lbl[0];
            calc_display[calc_display_len]   = '\0';
        }
        return;
    }

    if (lbl[0] == '.') {
        if (calc_has_dot) return;
        if (calc_fresh) {
            calc_display[0] = '0'; calc_display[1] = '.'; calc_display[2] = '\0';
            calc_display_len = 2;
            calc_fresh = false;
        } else if (calc_display_len < 14) {
            calc_display[calc_display_len++] = '.';
            calc_display[calc_display_len]   = '\0';
        }
        calc_has_dot = true;
        return;
    }

    if (lbl[0] == '=') {
        if (calc_op) {
            Num cur    = num_parse(calc_display);
            Num result = calc_apply(calc_acc, cur, calc_op);
            calc_set_display(result);
            calc_acc   = 0;
            calc_op    = 0;
            calc_fresh = true;
        }
        return;
    }

    // Operator: + - * /
    char op = lbl[0];
    if (calc_op) {
        Num cur  = num_parse(calc_display);
        calc_acc = calc_apply(calc_acc, cur, calc_op);
        calc_set_display(calc_acc);
    } else {
        calc_acc = num_parse(calc_display);
    }
    calc_op    = op;
    calc_fresh = true;
}
