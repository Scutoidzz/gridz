# C++ for Your OS Project

You don't need to know all of C++. Here's what you actually use:

## 1. Header Files (.hpp) and Implementation (.cpp)

```cpp
// compositor.hpp - Declaration (the contract)
class Compositor {
public:
    static void init();
    static int create_window(int x, int y, int width, int height, ...);
    static Window windows[MAX_WINDOWS];
    static int window_count;
private:
    static int focused_window;
};

// compositor.cpp - Implementation (the actual code)
int Compositor::window_count = 0;  // Initialize static variables here

void Compositor::init() {
    // actual code
}
```

**Key:** The `::` means "this belongs to class X". So `Compositor::init` means "the init function of the Compositor class".

**When adding new code:** declare it in .hpp, implement it in .cpp.

---

## 2. Static Methods (Functions Owned by a Class)

In Python, you might do:
```python
class Compositor:
    @staticmethod
    def init():
        pass
```

In C++, it's almost identical:
```cpp
class Compositor {
public:
    static void init();  // static = no object needed
};

// Call it without creating anything:
Compositor::init();
```

**Static = "belongs to the class itself, not individual objects"**

---

## 3. Structs (Data Containers)

```cpp
struct Window {
    int x, y;                      // position
    int width, height;             // size
    uint32_t background_color;     // a color value (unsigned 32-bit int)
    uint8_t alpha;                 // transparency (0-255)
    bool visible;                  // true/false
    bool focused;                  // true/false
    const char* title;             // pointer to text (see Pointers below)
    int z_index;                   // layer order
    uint32_t* content_buffer;      // optional pixel buffer
};

// Use it:
Window w;
w.x = 100;
w.y = 50;
w.visible = true;
```

**That's it.** Structs are just grouping data. Use `w.field = value` to set fields.

---

## 4. Arrays

```cpp
Window windows[4];              // array of 4 Window objects
windows[0].x = 100;             // set x of first window
windows[1].visible = true;      // set visible flag of second window
```

**Important:** Arrays are 0-indexed like Python. `windows[0]` is the first, `windows[3]` is the fourth.

---

## 5. Pointers (The Thing Everyone Finds Weird)

A pointer is just an **address in memory**. Two operators:
- `&` = "the address of this thing"
- `*` = "the thing at this address"

```cpp
int x = 42;
int* ptr = &x;      // ptr points to x
*ptr = 100;         // change x through the pointer (x is now 100)

// In function arguments:
void draw_rectangle(limine_framebuffer* fb, int x, int y, ...) {
    // fb is a pointer to a framebuffer
    // We use it like: fb->width, fb->height
}

// Pointer that points to nothing (safe):
uint32_t* buffer = nullptr;
if (buffer != nullptr) {  // check before using
    *buffer = 5;
}
```

**Arrow operator (`->`):** When you have a pointer to a struct, use `ptr->field` instead of `(*ptr).field`:

```cpp
limine_framebuffer* fb = get_framebuffer();
fb->width = 800;  // same as (*fb).width
```

---

## 6. const (Promises the Value Won't Change)

```cpp
const char* title = "My App";   // title points to text that won't change
const int MAX_WINDOWS = 4;      // MAX_WINDOWS will always be 4

void function(const Window* win) {  // promise: we won't modify win
    // can read win->x but can't set win->x = 5
}
```

**Don't overthink it:** `const` just means "don't change this". Use it to make promises about what your functions do.

---

## 7. Type Shortcuts You'll See

```cpp
uint32_t color = 0xFF0000;  // unsigned 32-bit = 0 to 4 billion (good for colors)
uint8_t alpha = 255;        // unsigned 8-bit = 0 to 255 (good for opacity)
int x = 100;                // signed 32-bit = -2 billion to +2 billion
bool visible = true;        // true or false
```

**For your project:** `uint32_t` for colors, `uint8_t` for small values (0-255), `int` for everything else.

---

## 8. Function Declarations vs Definitions

```cpp
// In .hpp (declaration = "this function exists")
void move_window(int window_id, int x, int y);

// In .cpp (definition = "here's what it does")
void move_window(int window_id, int x, int y) {
    if (window_id < 0 || window_id >= window_count) return;
    windows[window_id].x = x;
    windows[window_id].y = y;
}
```

**Pattern:** Declare in header, implement in .cpp. When you add features, follow this pattern.

---

## 9. Common Patterns in Your Code

### Pattern: Static Array Management
```cpp
class Manager {
    static int items[MAX_ITEMS];
    static int item_count;
    
public:
    static int create_item(...) {
        if (item_count >= MAX_ITEMS) return -1;  // full
        int id = item_count;
        items[id] = /* ... */;
        item_count++;
        return id;
    }
};
```

### Pattern: Looping Through Array
```cpp
for (int i = 0; i < item_count; i++) {
    items[i].do_something();
}
```

### Pattern: Safety Checks
```cpp
void do_something_with_window(int window_id) {
    if (window_id < 0 || window_id >= window_count) return;  // safe!
    windows[window_id].property = value;
}
```

---

## 10. Extern (Using Code from Other Files)

```cpp
extern "C" void serial_print(const char* s);  // defined elsewhere, use it here
extern Terminal* global_term;                 // global variable from another file
extern volatile int mouse_x;                  // the volatile keyword means hardware might change it
```

**When to use:** When you need something defined in another .cpp file.

---

## Quick Cheat Sheet

| C++ | Python equivalent |
|-----|------------------|
| `uint32_t x = 5;` | `x = 5` |
| `Window w; w.x = 10;` | `w = Window(); w.x = 10` |
| `Window* ptr = &w;` | `ptr = w` (but explicit about it being a reference) |
| `ptr->x = 5;` | `ptr.x = 5` |
| `static void func()` | `@staticmethod def func()` |
| `const char* s` | `s: str` (but immutable) |
| `nullptr` | `None` |
| `for (int i = 0; i < 5; i++)` | `for i in range(5):` |

---

## How to Add a Feature

Example: Add a function to hide all windows.

**Step 1:** Declare in .hpp
```cpp
class Compositor {
public:
    static void hide_all_windows();
};
```

**Step 2:** Implement in .cpp
```cpp
void Compositor::hide_all_windows() {
    for (int i = 0; i < window_count; i++) {
        windows[i].visible = false;
    }
}
```

**Step 3:** Use it
```cpp
Compositor::hide_all_windows();
```

---

## Common Mistakes

1. **Forgetting `static` in implementation**
   ```cpp
   // Wrong:
   void Compositor::init() { }
   
   // Right:
   void Compositor::init() { }
   // (the static is only in .hpp, not in .cpp)
   ```

2. **Using `nullptr` wrong**
   ```cpp
   // Wrong:
   uint32_t* buffer = nullptr;
   *buffer = 5;  // CRASH! pointer is null
   
   // Right:
   uint32_t* buffer = nullptr;
   if (buffer != nullptr) {
       *buffer = 5;
   }
   ```

3. **Array out of bounds**
   ```cpp
   // Wrong:
   Window windows[4];
   windows[4].x = 5;  // CRASH! index 4 doesn't exist (only 0-3)
   
   // Right:
   if (id >= 0 && id < MAX_WINDOWS) {
       windows[id].x = 5;
   }
   ```

---

## That's It

You now know 90% of the C++ you need for this project. The rest you'll learn by reading the existing code and modifying it.

**Next steps:**
- Pick a feature to add
- Find similar code in the repo
- Copy the pattern and modify it
- If you get a compiler error, google it—the error message is usually pretty clear
