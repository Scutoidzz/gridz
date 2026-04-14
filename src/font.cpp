#include "font.hpp"
#include "../../documentation/font8x8.h"

bool get_char_bitmap(char c, uint8_t out_bitmap[8]) {
    if ((uint8_t)c < 128) {
        for (int i = 0; i < 8; i++) {
            out_bitmap[i] = font8x8_basic[(uint8_t)c][i];
        }
        return true;
    }
    return false;
}
