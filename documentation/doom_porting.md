# Adding DOOM to Gridz Kernel

The `doom` command has been added to `src/commands.hpp`.

To run a full copy of DOOM in your kernel, you can use a project like [doomgeneric](https://github.com/ozkl/doomgeneric). It is designed to be easily compilable into small environments and OS kernels by stubbing out system dependencies.

## Steps Overview
1. **Pull `doomgeneric` source code** into your project via submodules or direct download.
2. **Implement minimum stubs** required by doomgeneric:
   - `DG_Init()`: Initialize your window/framebuffer
   - `DG_DrawFrame()`: Copy the DOOM backbuffer to your `limine_framebuffer`
   - `DG_SleepMs(uint32_t ms)`: Implement a hardware loop or PIT delay
   - `DG_GetTicksMs()`: Provide system time in milliseconds
   - `DG_GetKey(int* pressed, unsigned char* key)`: Feed keyboard state to Doom
3. **Compile the DOOM C engine** against your kernel build with a flat memory model (you'll need enough heap for DOOM to run, ideally a few MBs malloc stub).

## Command Integration Snippet
You can update `src/commands.hpp` manually using this snippet to add the `doom` command:

```cpp
    else if (match_cmd(buffer, "doom", 4)) {
        term->cursor_y += 8;
        term->cursor_x = 0;
        const char* msg = "Initializing DOOM (doomgeneric)...";
        for(int i=0; msg[i] != '\0'; i++) {
            term->draw_char(msg[i], term->cursor_x, term->cursor_y, 0xFF00FF);
            term->cursor_x += 8;
        }
        
        // Example call: Initialize your DOOM environment
        // doomgeneric_Create(term->fb);
        // doomgeneric_main(0, nullptr);
    }
```
