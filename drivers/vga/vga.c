/*
 * drivers/vga/vga.c — VGA text-mode driver
 *
 * Manages the 80×25 VGA text buffer at 0xB8000.
 */

#include <drivers/vga.h>
#include <miniOS/types.h>

#define VGA_MEMORY  ((volatile u16 *)0xB8000)

static size_t  vga_row;
static size_t  vga_column;
static u8      vga_color;
static volatile u16 *vga_buffer = VGA_MEMORY;

void vga_init(void)
{
    vga_row    = 0;
    vga_column = 0;
    vga_color  = vga_entry_color(VGA_LIGHT_GREY, VGA_BLACK);
    vga_buffer = VGA_MEMORY;
    vga_clear();
}

void vga_set_color(u8 color)
{
    vga_color = color;
}

void vga_clear(void)
{
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            vga_buffer[index]  = vga_entry(' ', vga_color);
        }
    }
    vga_row    = 0;
    vga_column = 0;
}

void vga_scroll(void)
{
    /* Move lines [1..24] up to [0..23] */
    for (size_t y = 1; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            vga_buffer[(y - 1) * VGA_WIDTH + x] =
                vga_buffer[y * VGA_WIDTH + x];
        }
    }
    /* Clear the last line */
    for (size_t x = 0; x < VGA_WIDTH; x++) {
        vga_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] =
            vga_entry(' ', vga_color);
    }
}

void vga_putchar(char c)
{
    if (c == '\n') {
        vga_column = 0;
        vga_row++;
    } else if (c == '\r') {
        vga_column = 0;
    } else if (c == '\t') {
        vga_column = (vga_column + 4) & ~3;
        if (vga_column >= VGA_WIDTH) {
            vga_column = 0;
            vga_row++;
        }
    } else if (c >= ' ') {
        const size_t index = vga_row * VGA_WIDTH + vga_column;
        vga_buffer[index]  = vga_entry((unsigned char)c, vga_color);
        vga_column++;
        if (vga_column >= VGA_WIDTH) {
            vga_column = 0;
            vga_row++;
        }
    }

    /* Scroll if needed */
    if (vga_row >= VGA_HEIGHT) {
        vga_scroll();
        vga_row    = VGA_HEIGHT - 1;
        vga_column = 0;
    }
}

void vga_write(const char *data, size_t size)
{
    for (size_t i = 0; i < size; i++)
        vga_putchar(data[i]);
}

void vga_puts(const char *str)
{
    while (*str)
        vga_putchar(*str++);
}

void vga_set_cursor(size_t row, size_t col)
{
    vga_row    = row;
    vga_column = col;
}
