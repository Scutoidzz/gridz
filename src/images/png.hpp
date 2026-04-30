#include "string.hpp"
#include "grep.hpp"

inline uint32_t swap_uint32(uint32_t val) {
    return ((val << 24) & 0xff000000) |
           ((val <<  8) & 0x00ff0000) |
           ((val >>  8) & 0x0000ff00) |
           ((val >> 24) & 0x000000ff);
}

struct PNGChunk {
    uint32_t length;
    char type[4];
    uint8_t* data;
    uint32_t crc;
};

struct PNGHeader {
    uint32_t width;
    uint32_t height;
    uint8_t bit_depth;
    uint8_t color_type;
    uint8_t compression_method;
    uint8_t filter_method;
    uint8_t interlace_method;
};

bool is_png(const uint8_t* data, size_t size);
int render_png(const char* filename, limine_framebuffer* fb);

int determine_image_type() {
    size_t size;
    uint8_t* data = (uint8_t*)fat32::read_file("image.png", &size);
    if (!data) return 0;

    if (size >= 8 && memcmp(data, "\x89PNG\r\n\x1a\n", 8) == 0) {
        return 1;
    }
    return 0;
}

