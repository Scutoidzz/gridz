#include "png.hpp"
#include "tinf.h"
#include "ui/ui.hpp"

extern uint64_t hhdm_offset;
bool deflate_decompress(uint8_t* compressed_data, size_t compressed_size, uint8_t* decompressed_data, size_t decompressed_size) {
    unsigned int dlen = (unsigned int)decompressed_size;
    int res = tinf_zlib_uncompress(decompressed_data, &dlen, compressed_data, (unsigned int)compressed_size);
    return res == TINF_OK;
}
void unfilter_scanline(uint8_t* current, uint8_t* previous, uint8_t filter_type, int bpp, int width) {
    for (int i = 0; i < width * bpp; i++) {
        uint8_t a = (i >= bpp) ? current[i - bpp] : 0;
        uint8_t b = (previous) ? previous[i] : 0;
        uint8_t c = (previous && i >= bpp) ? previous[i - bpp] : 0;
        switch (filter_type) {
            case 1: current[i] += a; break; 
            case 2: current[i] += b; break; 
            case 3: current[i] += (a + b) / 2; break; 
            case 4: { 
                int p = (int)a + (int)b - (int)c;
                int pa = (p > a) ? p - a : a - p;
                int pb = (p > b) ? p - b : b - p;
                int pc = (p > c) ? p - c : c - p;
                if (pa <= pb && pa <= pc) current[i] += a;
                else if (pb <= pc) current[i] += b;
                else current[i] += c;
                break;
            }
            default: break; // None
        }
    }
}

int render_png(const char* filename, limine_framebuffer* fb) {
    size_t file_size;
    uint8_t* data = (uint8_t*)fat32::read_file(filename, &file_size);
    if (!data) return -1;

    if (memcmp(data, "\x89PNG\r\n\x1a\n", 8) != 0) return -2;

    PNGHeader header;
    uint8_t* idat_buf = nullptr;
    size_t idat_size = 0;
    size_t offset = 8;

    while (offset < file_size) {
        uint32_t len = swap_uint32(*(uint32_t*)(data + offset));
        char* type = (char*)(data + offset + 4);
        uint8_t* chunk_data = data + offset + 8;

        if (memcmp(type, "IHDR", 4) == 0) {
            header.width = swap_uint32(*(uint32_t*)chunk_data);
            header.height = swap_uint32(*(uint32_t*)(chunk_data + 4));
            header.bit_depth = chunk_data[8];
            header.color_type = chunk_data[9];
        } else if (memcmp(type, "IDAT", 4) == 0) {
            idat_size += len;
        } else if (memcmp(type, "IEND", 4) == 0) {
            break;
        }
        offset += 8 + len + 4;
    }

    if (idat_size == 0 || header.width == 0 || header.height == 0) return -3;

    idat_buf = (uint8_t*)malloc(idat_size);
    if (!idat_buf) return -4;

    offset = 8;
    size_t current_idat_offset = 0;
    while (offset < file_size) {
        uint32_t len = swap_uint32(*(uint32_t*)(data + offset));
        char* type = (char*)(data + offset + 4);
        uint8_t* chunk_data = data + offset + 8;

        if (memcmp(type, "IDAT", 4) == 0) {
            memcpy(idat_buf + current_idat_offset, chunk_data, len);
            current_idat_offset += len;
        } else if (memcmp(type, "IEND", 4) == 0) {
            break;
        }
        offset += 8 + len + 4;
    }

    // PNG color type 2 is RGB (3 bytes per pixel), 6 is RGBA (4 bytes per pixel)
    int bpp = (header.color_type == 6) ? 4 : 3;
    size_t decompressed_size = header.height * (1 + header.width * bpp);
    uint8_t* decompressed = (uint8_t*)malloc(decompressed_size);
    if (!decompressed) {
        free(idat_buf);
        return -4;
    }

    if (!deflate_decompress(idat_buf, idat_size, decompressed, decompressed_size)) {
        free(decompressed);
        free(idat_buf);
        return -5;
    }

    uint8_t* prev_line = nullptr;
    for (uint32_t y = 0; y < header.height; y++) {
        uint8_t* line = decompressed + y * (1 + header.width * bpp);
        uint8_t filter_type = *line;
        uint8_t* pixel_data = line + 1;

        unfilter_scanline(pixel_data, prev_line, filter_type, bpp, header.width);
        prev_line = pixel_data;

        // Draw to framebuffer
        for (uint32_t x = 0; x < header.width; x++) {
            uint8_t r = pixel_data[x * bpp];
            uint8_t g = pixel_data[x * bpp + 1];
            uint8_t b = pixel_data[x * bpp + 2];
            uint32_t color = (r << 16) | (g << 8) | b;
            
            uint32_t* fb_ptr = (uint32_t*)((uint64_t)fb->address);
            fb_ptr[y * (fb->pitch / 4) + x] = color;
        }
    }

    free(idat_buf);
    free(decompressed);
    return 0;
}
