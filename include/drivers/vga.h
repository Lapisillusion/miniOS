#ifndef _MINIOS_VGA_H
#define _MINIOS_VGA_H

#include <miniOS/types.h>

/* VGA text mode dimensions */
#define VGA_WIDTH   80
#define VGA_HEIGHT  25

/* VGA colors */
enum vga_color {
    VGA_BLACK         = 0,
    VGA_BLUE          = 1,
    VGA_GREEN         = 2,
    VGA_CYAN          = 3,
    VGA_RED           = 4,
    VGA_MAGENTA       = 5,
    VGA_BROWN         = 6,
    VGA_LIGHT_GREY    = 7,
    VGA_DARK_GREY     = 8,
    VGA_LIGHT_BLUE    = 9,
    VGA_LIGHT_GREEN   = 10,
    VGA_LIGHT_CYAN    = 11,
    VGA_LIGHT_RED     = 12,
    VGA_LIGHT_MAGENTA = 13,
    VGA_LIGHT_BROWN   = 14,
    VGA_WHITE         = 15,
};

static inline u8 vga_entry_color(enum vga_color fg, enum vga_color bg)
{
    return (u8)(fg | (bg << 4));
}

static inline u16 vga_entry(unsigned char c, u8 color)
{
    return (u16)((u16)c | ((u16)color << 8));
}

/* VGA output functions */
void vga_init(void);
void vga_clear(void);
void vga_putchar(char c);
void vga_write(const char *data, size_t size);
void vga_puts(const char *str);
void vga_set_color(u8 color);
void vga_set_cursor(size_t row, size_t col);
void vga_scroll(void);

#endif /* _MINIOS_VGA_H */
