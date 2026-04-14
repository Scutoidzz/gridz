// No_STD because I'm not a std guy

#![no_main]

use core::panic::PanicInfo;

#[panic_handler]
fn panic(_info: &PanicInfo) -> {
    loop {}
}

fn draw_to_screen -> {
    // Framebugger was a typo but I'm keeping it
    pub struct FrameBugger {
        pub buffer: &'static mut[u8],
        pub width: usize,
        pub height: usize, 
        pub b_b_p: usize,
        pub stride: usize,        
    }
    impl FrameBugger {
        pub fn write_pixel(&mut self, x: usize, y: usize, color: [u8; 4]) {
            let pixel_offset = y * self.stride + x;
            let byte_offset = pixel_offset * self.b_b_p;
            let pixel_bytes = color.to_le_bytes();
            self.buffer[byte_offset..byte_offset + 4].copy_from_slice(&pixel_bytes);
        }
    }
}