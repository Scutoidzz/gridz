#include <stdint.h>
#include <stddef.h>

extern "C" {
#include <port/micropython_embed.h>
}

#define PY_TERM_W              400
#define PY_TERM_H              300
#define PY_LINE_W              100
#define PY_N_LINES             20

static uint32_t py_buf[PY_TERM_W * (PY_TERM_H - MAX_TITLEBAR_HEIGHT)];
extern int python_window_id;

static char py_output_lines[PY_N_LINES][PY_LINE_W];
static int py_output_head = 0;
static int py_input_buf[PY_LINE_W];
static int py_input_len = 0;

static bool py_initialized = false;
static char py_heap[32 * 1024];

// File to auto-run when the window opens (set by start menu before python_open = true)
char py_launch_file[128] = {};

// Input mode state
static bool py_waiting_for_input = false;
static char py_input_line[PY_LINE_W];
static int py_input_line_len = 0;
static bool py_input_ready = false;

extern "C" void python_term_write(const char *str, size_t len) {
    if (!str || len == 0) return;

    int pos = 0;
    while (pos < (int)len) {
        char *line = py_output_lines[py_output_head];
        int line_len = 0;
        while (line[line_len]) line_len++;

        int space_left = PY_LINE_W - 1 - line_len;
        int to_copy = (int)len - pos;
        if (to_copy > space_left) to_copy = space_left;

        for (int i = 0; i < to_copy; i++) {
            if (str[pos + i] == '\n') {
                line[line_len + i] = '\0';
                py_output_head = (py_output_head + 1) % PY_N_LINES;
                py_output_lines[py_output_head][0] = '\0';
                pos += i + 1;
                to_copy = 0;
                break;
            }
            line[line_len + i] = str[pos + i];
        }

        if (to_copy > 0) {
            line[line_len + to_copy] = '\0';
            pos += to_copy;
        }

        if (pos < (int)len && str[pos] == '\n') {
            py_output_head = (py_output_head + 1) % PY_N_LINES;
            py_output_lines[py_output_head][0] = '\0';
            pos++;
        }
    }
}


extern "C" void python_start_input_mode(void) {
    py_waiting_for_input = true;
    py_input_line_len = 0;
    py_input_line[0] = '\0';
}

extern "C" const char* python_get_input_line(void) {
    py_input_line[py_input_line_len] = '\0';
    return py_input_line;
}

extern "C" bool python_input_ready(void) {
    return py_input_ready;
}

extern "C" void python_input_clear(void) {
    py_input_ready = false;
    py_waiting_for_input = false;
    py_input_line_len = 0;
}

static void python_execute_line() {
    if (py_input_len == 0) {
        python_term_write("\n", 1);
        return;
    }

    char input_str[PY_LINE_W];
    for (int i = 0; i < py_input_len; i++) {
        input_str[i] = (char)py_input_buf[i];
    }
    input_str[py_input_len] = '\0';

    python_term_write(">>> ", 4);
    python_term_write(input_str, py_input_len);
    python_term_write("\n", 1);

    if (!py_initialized) {
        int stack_top;
        mp_embed_init(&py_heap[0], sizeof(py_heap), &stack_top);
        py_initialized = true;
    }

    mp_embed_exec_str(input_str);

    py_input_len = 0;
}

extern "C" void python_handle_key(char c) {
    if (py_waiting_for_input) {
        // Handle input mode (waiting for user input from input() call)
        if (c == '\n' || c == '\r') {
            py_input_line[py_input_line_len] = '\0';
            python_term_write(py_input_line, py_input_line_len);
            python_term_write("\n", 1);
            py_input_ready = true;
        } else if (c == 8 || c == 127) {
            if (py_input_line_len > 0) py_input_line_len--;
        } else if (c >= 32 && c < 127) {
            if (py_input_line_len < (int)sizeof(py_input_line) - 1) {
                py_input_line[py_input_line_len++] = c;
            }
        }
    } else {
        // Handle REPL mode (command line input)
        if (c == '\n' || c == '\r') {
            python_execute_line();
        } else if (c == 8 || c == 127) {
            if (py_input_len > 0) py_input_len--;
        } else if (c >= 32 && c < 127) {
            if (py_input_len < (int)sizeof(py_input_buf) - 1) {
                py_input_buf[py_input_len++] = c;
            }
        }
    }
}

void draw_python_window(limine_framebuffer *fb) {
    if (!python_open) {
        if (python_window_id >= 0) {
            Compositor::destroy_window(python_window_id);
            python_window_id = -1;
        }
        return;
    }

    int W = (int)fb->width;
    int H = (int)fb->height;

    const int ww = PY_TERM_W, wh = PY_TERM_H;
    int wx = W / 2 - ww / 2;
    int wy = H / 2 - wh / 2;

    if (python_window_id < 0) {
        python_window_id = Compositor::create_window(wx, wy, ww, wh, "Python",
                                                     0x0D1520, 240, 10);
        Compositor::windows[python_window_id].content_buffer = py_buf;
        Compositor::windows[python_window_id].content_buffer_w = PY_TERM_W;
        Compositor::windows[python_window_id].content_buffer_h =
            PY_TERM_H - MAX_TITLEBAR_HEIGHT;

        // Auto-run a queued file (set before python_open = true)
        if (py_launch_file[0]) {
            if (!py_initialized) {
                int stack_top;
                mp_embed_init(&py_heap[0], sizeof(py_heap), &stack_top);
                py_initialized = true;
            }
            // Build: import files; files.load("filename")
            char cmd[160];
            int ci = 0;
            const char *prefix = "import files\nfiles.load(\"";
            while (prefix[ci] && ci < 140) { cmd[ci] = prefix[ci]; ci++; }
            for (int i = 0; py_launch_file[i] && ci < 155; i++, ci++)
                cmd[ci] = py_launch_file[i];
            cmd[ci++] = '"'; cmd[ci++] = ')'; cmd[ci] = '\0';
            mp_embed_exec_str(cmd);
            py_launch_file[0] = '\0';
        }
    }

    limine_framebuffer fake_fb;
    fake_fb.address = py_buf;
    fake_fb.width = PY_TERM_W;
    fake_fb.height = PY_TERM_H - MAX_TITLEBAR_HEIGHT;
    fake_fb.pitch = PY_TERM_W * 4;

    draw_rectangle(&fake_fb, 0, 0, fake_fb.width, fake_fb.height, 0x0D1520);

    int y = 4;
    for (int i = 0; i < PY_N_LINES; i++) {
        int idx = (py_output_head + 1 + i) % PY_N_LINES;
        if (py_output_lines[idx][0] != '\0') {
            draw_string(&fake_fb, 4, y, py_output_lines[idx], 0xDDEEFF);
        }
        y += 12;
    }

    char input_display[PY_LINE_W + 10];
    if (py_waiting_for_input) {
        input_display[0] = '?';
        input_display[1] = ' ';
        for (int i = 0; i < py_input_line_len && i < (int)sizeof(input_display) - 3; i++) {
            input_display[2 + i] = py_input_line[i];
        }
        input_display[2 + py_input_line_len] = '|';
        input_display[2 + py_input_line_len + 1] = '\0';
    } else {
        input_display[0] = '>';
        input_display[1] = ' ';
        for (int i = 0; i < py_input_len && i < (int)sizeof(input_display) - 3; i++) {
            input_display[2 + i] = (char)py_input_buf[i];
        }
        input_display[2 + py_input_len] = '|';
        input_display[2 + py_input_len + 1] = '\0';
    }

    draw_string(&fake_fb, 4, fake_fb.height - 16, input_display, 0x7FCBFF);
}
