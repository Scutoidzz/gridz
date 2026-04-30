#include "ui/ui.hpp"
#include "setup.hpp"
#include "limine.h"
#include "png.hpp"

void name_interface(limine_framebuffer* fb) {
    if (!fb) return;

    render_png("wallpaper.png", fb);

    int card_width = (fb->width * 3) / 4;
    int card_height = (fb->height * 3) / 4;
    int card_x = (fb->width - card_width) / 2;
    int card_y = (fb->height - card_height) / 2;

    draw_rectangle_alpha(fb, card_x + 8, card_y + 8, card_width, card_height, 0x000000, 40);

    draw_rectangle(fb, card_x, card_y, card_width, card_height, 0xFFFFFF);
}