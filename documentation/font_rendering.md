# Font Rendering on UEFI Framebuffers

Since UEFI provides a raw grid of pixels, text must be rendered manually by plotting those pixels according to a pattern. This pattern is called a **Bitmap Font**.

## How Bitmap Fonts Work
Standard minimal fonts use an 8x8 grid. This translates perfectly to an array of 8 integers (bytes) per character:
- 1 byte has 8 bits.
- 1 array has 8 bytes (rows).
- `1` means "draw a pixel", `0` means "leave background".

To draw the character, you iterate over the 8 bytes. For each byte, you iterate over its 8 bits. If the bit is set to `1` (tested via a bitwise AND mask), you call `write_pixel()` at `(X + bit_column, Y + byte_row)`.

## Separate Modules
You should absolutely separate this into a new file, like `font.rs`. A full ASCII character set (128 characters) takes roughly 1,024 lines of binary data.
Keeping it separate from `main.rs` keeps your core screen logic clean. 

## Crates vs Custom
Most Rust OS developers pull in an existing font array by using a `no_std` crate like `font8x8` rather than typing out the hex matrices by hand, but implementing a few manual characters is an excellent learning exercise!

## References
- OSDev Font Tutorial: https://wiki.osdev.org/VGA_Fonts
- font8x8 Crate (great to look at for how they store mapping): https://docs.rs/font8x8/latest/font8x8/
