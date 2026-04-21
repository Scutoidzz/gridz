# Gridz OS: The Manual for Independent Development

This document is your guide to building and maintaining Gridz OS without external assistance. It covers the core architecture, the graphics system, and how to add new features.

---

## 1. The Foundation: How Gridz Boots
Gridz uses **Limine** as a bootloader. Limine does the "dirty work" of switching the CPU to 64-bit mode and setting up a graphics framebuffer.

- **Entry Point:** `src/main.cpp` -> `_start()`.
- **The Framebuffer:** Limine provides a pointer to memory (`fb->address`). Writing a color value (like `0xFFFFFF`) to a specific offset in this memory makes a pixel appear on the screen.
- **The Stride:** Screens aren't just `width * height`. The `fb->pitch` tells you how many bytes are in one horizontal line. This is crucial for calculating the address of a pixel at `(x, y)`:
  `location = (y * (pitch / 4)) + x` (if using a 32-bit pointer).

## 2. The Graphics Engine (`src/ui/`)
Your UI isn't a "library"; it's a series of loops that write numbers to memory.

### Drawing a Pixel
Every shape you draw eventually calls a loop that looks like this:
```cpp
uint32_t *fb_ptr = (uint32_t *)fb->address;
fb_ptr[y * (fb->pitch / 4) + x] = color;
```

### Alpha Blending (Transparency)
In `comp.cpp`, you have `draw_rectangle_alpha`. The formula for transparency is:
`Result = (Alpha * SourceColor + (255 - Alpha) * DestinationColor) / 255`
This takes the new color and the color already on the screen, mixes them based on the alpha value, and writes the result back.

## 3. Interrupts & Input (`src/idt.hpp`)
When you press a key, the hardware sends a signal to the CPU. The **IDT (Interrupt Descriptor Table)** tells the CPU which function to run.

1. **Keyboard:** Port `0x60` is the data port. When an interrupt happens, `inb(0x60)` reads the "scancode".
2. **Translation:** The `kbd_map` in `io.hpp` turns that raw scancode (e.g., `0x1E`) into a character (e.g., `'a'`).

## 4. How to Add a New Command
To add a feature to the terminal:
1. Open `src/commands.hpp`.
2. Find `execute_command`.
3. Add a new `else if (match_cmd(buffer, "your_cmd", length))`.
4. Use `term->print()` to talk back to the user.

## 5. How to Add a New UI Component
1. **Define it:** Add a function signature in `src/ui/components/comp.hpp`.
2. **Implement it:** In `src/ui/components/comp.cpp`, write the logic. 
3. **Use it:** Call it from `mainui.cpp`.

## 6. Debugging Without AI
When the screen goes black and the OS crashes:
- **Serial Debugging:** You have `serial_print` in `main.cpp`. Use it! Since it prints to your terminal (via QEMU's `-serial stdio`), it works even if the screen is broken.
- **The "Blink" Test:** If you aren't sure if a piece of code is running, draw a bright red rectangle in that function. If you see red, the code reached that point.

## 7. Essential Resources
- **OSDev Wiki (wiki.osdev.org):** The primary resource for OS development.
- **Limine Protocol:** [https://github.com/limine-bootloader/limine/blob/v5.x-binary/PROTOCOL.md](https://github.com/limine-bootloader/limine/blob/v5.x-binary/PROTOCOL.md)
- **C++ Reference:** [cppreference.com](https://cppreference.com) for language syntax.
