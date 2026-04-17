# Building a Command System

To organize your terminal commands cleanly, you can create a dedicated file that takes the `command_buffer` input and runs the corresponding logic. This prevents your `terminal.hpp` from becoming overly cluttered.

## 1. Command logic storage (`src/commands.hpp`)
Create a new header file to house the command execution logic. You can include your `Terminal` or raw framebuffer manipulations here.

```cpp
#pragma once

// A simple manual search method since <string.h> is unavailable
bool match_cmd(const char* buf, const char* target, int len) {
    for (int i = 0; i < len; i++) {
        if (buf[i] != target[i]) return false;
    }
    return true;
}

// Pass everything needed to mutate the screen
void execute_command(const char* buffer, Terminal* term, sboot_framebuffer* fb) {
    if (match_cmd(buffer, "clear", 6)) {
        // Clear screen logic...
    } 
    else if (match_cmd(buffer, "help", 5)) {
        term->print_char('\n');
        const char* msg = "Commands: clear, focus, help";
        for (int i = 0; msg[i] != '\0'; i++) term->print_char(msg[i]);
    }
}
```

## 2. Updating your Terminal (`src/terminal.hpp`)
You will need to maintain a stateful buffer inside the `Terminal` class to keep track of characters until `\n` is evaluated.

Inside `class Terminal`:
```cpp
private:
    char command_buffer[64];
    int cmd_len = 0;
```

Modify the `print_char` conditions to track backspaces and store standard inputs:

```cpp
void print_char(char c) {
    if (c == '\n') {
        command_buffer[cmd_len] = '\0';
        execute_command(command_buffer, this, fb);
        cmd_len = 0; // Wipe buffer
        newline();
    } else if (c == 8) { // Backspace
        if (cursor_x > 8) { 
            cursor_x -= 8;
            draw_char(' ', cursor_x, cursor_y, 0x000000);
            if (cmd_len > 0) cmd_len--; // Remove from buffer
        }
    } else {
        draw_char(c, cursor_x, cursor_y, 0xFFFFFF);
        if (cmd_len < 63) command_buffer[cmd_len++] = c; // Add to buffer
        cursor_x += 8;
        if (cursor_x >= (int)fb->width) newline();
    }
}
```
