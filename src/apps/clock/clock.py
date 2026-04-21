import gridz_ui

def main():
    fb = gridz_ui.get_framebuffer()
    gridz_ui.draw_rectangle(fb, 200, 200, 400, 300, 0x333333)
    gridz_ui.draw_string(fb, 250, 250, "Working Clock App", 0xFFFFFF)
    gridz_ui.draw_string(fb, 250, 280, "Time: There's no RTC support, what were you expecting?", 0x00FF00)
    gridz_ui.draw_string(fb, 250, 310, "Deaduzz, what were you expecting? a working clock app", 0x00FF00)
    

if __name__ == "__main__":
    main()
