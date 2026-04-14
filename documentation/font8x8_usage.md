# Using the `font8x8` Crate

The `font8x8` library provides hardcoded arrays mappings characters directly to their 8x8 pixel representations without using standard library allocations (it is `no_std` compatible).

## Key Components

1. **`UnicodeFonts` Trait**: You must bring this into scope (`use font8x8::UnicodeFonts;`) to unlock the `.get()` method.
2. **`BASIC_FONTS` Constants**: This constant array encompasses the standard ASCII printable characters.
3. **Return Type**: The `.get(char)` method returns an `Option<[u8; 8]>`. It will be `Some` if the library has a map for that character, and `None` if you ask for an unsupported character.

## Extended Sets
If you need characters beyond standard english ASCII (e.g. Greek, Box Drawing characters), `font8x8` also offers `LATIN_FONTS`, `GREEK_FONTS`, and `BOX_UNICODE` depending on what you import.

## Reference
- Crate Documentation: https://docs.rs/font8x8/latest/font8x8/
