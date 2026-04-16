# C++ Kernel Code Cleanup Suggestions

To improve readability, scalability, and maintainability in your `main.cpp`, consider separating different concerns into their own modules. Currently, hardware I/O, keyboard mapping, framebuffer graphics, and the main kernel loop are all consolidated into one file.

## 1. Encapsulate Terminal State
Group together the loose `cursor_x` and `cursor_y` variables and the `limine_framebuffer` pointer into a `Terminal` class or struct. This approach prevents global state leakage and allows you to easily add new methods (like `clear_screen`, `newline`, or `backspace`).

## 2. Isolate Hardware interaction
Move the low-level `inb` function and the `kbd_map` array to an `io.hpp` or `keyboard.hpp` file. This abstraction hides the intricacies of PS/2 port polling from the core logic within `_start`.

## 3. Implement a Proper Keyboard Driver
Currently, the main loop manually polls port `0x60`. Eventually, you should handle this in a dedicated keyboard driver module, perhaps utilizing CPU interrupts (IDT) rather than simple polling. 

## External Documentation References
For more robust OS Dev architecture and C++ kernel structuring, see:
* [OSDev Wiki: C++ Bare Bones](https://wiki.osdev.org/C_PlusPlus_Bare_Bones)
* [OSDev Wiki: Meaty Skeleton (Structure)](https://wiki.osdev.org/Meaty_Skeleton)
* [OSDev Wiki: Text UI](https://wiki.osdev.org/Text_UI)
