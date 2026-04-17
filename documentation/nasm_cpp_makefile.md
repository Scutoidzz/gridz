# Mixing C++ and NASM Assembly in Makefiles
When building a custom kernel or mixing ASM with C++, you must assemble your `.asm` files into object files (`.o`) using an assembler like `nasm`, and compile your C++ files into `.o` files using `g++` (or `gcc`). Finally, the linker ties all those `.o` files together into the final executable.

### Makefile Syntax
You can set up a pattern rule specifically for compiling `.asm` files. Note that your `OBJS` variable must include the objects generated from BOTH the C++ sources and the ASM sources.

```makefile
# Custom target for NASM files
%.o: %.asm
	nasm -f elf64 $< -o $@
```

### Extern "C"
If your C++ code needs to call an assembly function, it must handle the C++ name mangling by using `extern "C"`. Without `extern "C"`, C++ looks for a mangled name like `_Z12keyboard_isrv` instead of just `keyboard_isr`, causing an `undefined reference` during linking.

```cpp
// In your main.cpp
extern "C" void keyboard_isr();
```

In your assembly code, ensure you expose the function using `global`:

```nasm
; In interrups.asm
global keyboard_isr
keyboard_isr:
    ; ...
    iretq
```
