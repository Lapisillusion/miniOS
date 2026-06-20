/*
 * lib/printf.c — Kernel printf that fans out to serial + VGA.
 */

#include <miniOS/types.h>
#include <lib/printf.h>
#include <drivers/vga.h>
#include <drivers/serial.h>

/* ---- kputs — plain string to both consoles --------------------------- */

void kputs(const char *str)
{
    serial_puts(str);
    vga_puts(str);
}

/* ---- kvprintf — va_list → static buffer → both consoles -------------- */

void kvprintf(const char *fmt, __builtin_va_list args)
{
    static char buf[1024];

    vsnprintf(buf, sizeof(buf), fmt, args);
    serial_puts(buf);
    vga_puts(buf);
}

/* ---- kprintf — the one you actually call ------------------------------ */

void kprintf(const char *fmt, ...)
{
    __builtin_va_list args;
    __builtin_va_start(args, fmt);
    kvprintf(fmt, args);
    __builtin_va_end(args);
}
