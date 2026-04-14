# Modern UEFI Graphics in Rust OS Development

VGA Text Mode is obsolete. Modern systems boot via UEFI, which provides a **Framebuffer** (a large, contiguous array of memory where each index corresponds to a pixel on the screen).

## How it works:
1. **Bootloader:** The bootloader (e.g., the `bootloader` crate in Rust) queries the UEFI firmware for the `EFI_GRAPHICS_OUTPUT_PROTOCOL` (GOP).
2. **Handoff:** The bootloader sets the display resolution and passes the starting memory address of the framebuffer to your Rust kernel.
3. **Rendering:** To display text, your kernel must include a font (like a bitmap array) and iterate through it, writing individual pixels (RGB values) into the framebuffer array.

## Popular Libraries
- **`font8x8`**: A 1-bit, 8x8 font very commonly used for rendering text in Rust framebuffers.
- **`bootloader`**: The de-facto standard Rust bootloader crate that sets up the UEFI environment and gives you the `&mut [u8]` framebuffer.

## Essential Links
- OSDev Wiki GOP: https://wiki.osdev.org/GOP
- Rust Bootloader Crate Docs: https://docs.rs/bootloader
- Philipp Oppermann's Framebuffer Guide: https://os.phil-opp.com/vga-text-mode/#what-about-uefi
